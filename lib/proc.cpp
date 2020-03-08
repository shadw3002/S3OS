#include "syscall.h"
#include "type.h"
#include "const.h"
#include "protect.h"
#include "tty.h"
#include "console.h"
#include "string.h"
#include "proc.h"
#include "systask.h"
#include "string.h"
#include "clock.h"
#include "interruption.h"
#include "assert.h"
#include "stdio.h"
#include "hd_driver.h"
#include "filesystem.h"
#include "task.h"

Tss		tss;

proc::Process*	p_proc_ready;




namespace proc {
    u32 proc_num;

    Process	proc_table[kMaxProc];

    char   proc_stack[kProcStackSize];

    u32 used_proc_stack;


    /*****************************************************************************
     *				  ldt_seg_linear
    *****************************************************************************/
    /**
     * <Ring 0~1> Calculate the linear address of a certain segment of a given
     * proc.
     *
     * @param p   Whose (the proc ptr).
     * @param idx Which (one proc has more than one segments).
     *
     * @return  The required linear address.
     *****************************************************************************/
    int ldt_seg_linear(Process* p, int idx)
    {
        Descriptor * d = &p->ldts[idx];

        return d->base_high << 24 | d->base_mid << 16 | d->base_low;
    }

    /*****************************************************************************
     *				  va2la
    *****************************************************************************/
    /**
     * <Ring 0~1> Virtual addr --> Linear addr.
     *
     * @param pid  PID of the proc whose address is to be calculated.
     * @param va   Virtual address.
     *
     * @return The linear address for the given virtual address.
     *****************************************************************************/
    void* va2la(int pid, void* va)
    {
        Process* p = &proc_table[pid];

        u32 seg_base = ldt_seg_linear(p, 1);
        u32 la = seg_base + (u32)va;

        if (pid < proc_num) {
            // assert(la == (u32)va);
        }

        return (void*)la;
    }

    u32 proc2pid(Process* p)
    {
        return p - proc_table;
    }

    void clone(Process* p)
    {
        Process*	p_proc		= proc_table + proc_num;
        char*		p_task_stack	= proc_stack + kProcStackSize - used_proc_stack;

        memcpy(p_proc, p, sizeof(Process));

        p_proc->pid = proc_num;
        p_proc->stack = p_task_stack - p->stack_size;
        p_proc->regs.esp += (u32)(p_proc->stack - p->stack);

        memcpy(p_proc->stack, p->stack, p->stack_size);

        p->is_father = true;
        p_proc->is_father = false;
        p_proc->thread_list = p->thread_list;
        p->thread_list = p_proc->pid;

        proc_num++;
    }

    void add_proc(task::Task* p_task, u32 ring)
    {
        Process*	p_proc		= proc_table + proc_num;
        char*		p_task_stack	= proc_stack + kProcStackSize - used_proc_stack;

        u8              privilege;
        u8              rpl;
        int             eflags;
        u32             nr_tty;

        u32 prio;

        if (ring == 1) {
            privilege = PRIVILEGE_TASK;
            rpl       = RPL_TASK;
            eflags    = 0x1202; /* IF=1, IOPL=1, bit 2 is always 1 */
            prio      = 15;
            nr_tty     = 0;
        }
        else {
            privilege = PRIVILEGE_USER;
            rpl       = RPL_USER;
            eflags    = 0x202; /* IF=1, bit 2 is always 1 */
            prio      = 5;
            nr_tty     = 2;
        }

        strcpy(p_proc->name, p_task->name);	// name of the process
        p_proc->pid = proc_num;			// pid

        p_proc->ldt_sel = SELECTOR_LDT_FIRST + (1 << 3) * proc_num;

        memcpy(&p_proc->ldts[0], &gdt[SELECTOR_KERNEL_CS >> 3],
            sizeof(Descriptor));
        p_proc->ldts[0].attr1 = DA_C | privilege << 5;
        memcpy(&p_proc->ldts[1], &gdt[SELECTOR_KERNEL_DS >> 3],
            sizeof(Descriptor));
        p_proc->ldts[1].attr1 = DA_DRW | privilege << 5;
        p_proc->regs.cs	= (0 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
        p_proc->regs.ds	= (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
        p_proc->regs.es	= (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
        p_proc->regs.fs	= (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
        p_proc->regs.ss	= (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
        p_proc->regs.gs	= (SELECTOR_KERNEL_GS & SA_RPL_MASK) | rpl;

        p_proc->regs.eip = (u32)p_task->initial_eip;
        p_proc->regs.esp = (u32)p_task_stack;
        p_proc->regs.eflags = eflags;

        p_proc->ticks = p_proc->priority = prio;

        p_proc->state = 0;
        p_proc->msg = 0;
        p_proc->recvfrom = NO_TASK;
        p_proc->sendto = NO_TASK;
        p_proc->has_int_msg = 0;
        p_proc->q_sending = 0;
        p_proc->next_sending = 0;

        if (proc_num == 5) p_proc->nr_tty = 0;

        used_proc_stack += p_task->stacksize;
        p_proc->stack_size = p_task->stacksize;
        p_proc->stack = p_task_stack - p_proc->stack_size;

        // 设置ldt
        add_ldt_desc(proc_num);

        p_proc->is_father = true;
        p_proc->thread_list = proc_num;

        proc_num++;
    }

    void init_proc_table()
    {
        used_proc_stack = 0;
        proc_num = 0;
    }

    void schedule()
    {
        Process*	p;
        int		greatest_ticks = 0;

        while (!greatest_ticks) {
            for (p = proc_table; p < proc_table + proc_num; p++) {
                if (p->state == READY) {
                    if (p->ticks > greatest_ticks) {
                        greatest_ticks = p->ticks;
                        p_proc_ready = p;
                    }
                }
            }

            if (!greatest_ticks)
                for (p = proc_table; p < proc_table + proc_num; p++)
                    if (p->state == READY)
                        p->ticks = p->priority;
        }
    }
}



namespace proc::ipc {
    void reset_msg(Message* msg)
    {
        memset(msg, 0, sizeof(Message));
    }





    /*****************************************************************************
     *                                block
     *****************************************************************************/
    /**
     * <Ring 0> This routine is called after `state' has been set (!= 0), it
     * calls `schedule()' to choose another proc as the `proc_ready'.
     *
     * @attention This routine does not change `state'. Make sure the `state'
     * of the proc to be blocked has been set properly.
     *
     * @param p The proc to be blocked.
     *****************************************************************************/
    static void block(Process* p)
    {
        assert(p->state != ProcState::READY);
        schedule();
    }

    /*****************************************************************************
     *                                unblock
     *****************************************************************************/
    /**
     * <Ring 0> This is a dummy routine. It does nothing actually. When it is
     * called, the `state' should have been cleared (== 0).
     *
     * @param p The unblocked proc.
     *****************************************************************************/
    static void unblock(Process* p)
    {
        assert(p->state == ProcState::READY);
    }

    /*****************************************************************************
     *                                deadlock
     *****************************************************************************/
    /**
     * <Ring 0> Check whether it is safe to send a message from src to dest.
     * The routine will detect if the messaging graph contains a cycle. For
     * instance, if we have procs trying to send messages like this:
     * A -> B -> C -> A, then a deadlock occurs, because all of them will
     * wait forever. If no cycles detected, it is considered as safe.
     *
     * @param src   Who wants to send message.
     * @param dest  To whom the message is sent.
     *
     * @return Zero if success.
     *****************************************************************************/
    static int deadlock(int src, int dest)
    {
        Process* p = proc_table + dest;
        while (1) {
            if (p->state == ProcState::SENDING) {
                if (p->sendto == src) {
                    /* print the chain */
                    p = proc_table + dest;
                    printf("=_=%s", p->name);
                    do {
                        assert(p->msg);
                        p = proc_table + p->sendto;
                        printf("->%s", p->name);
                    } while (p != proc_table + src);
                    printf("=_=");

                    return 1;
                }
                p = proc_table + p->sendto;
            }
            else {
                break;
            }
        }
        return 0;
    }


    /*****************************************************************************
     *                                msg_send
     *****************************************************************************/
    /**
     * <Ring 0> Send a message to the dest proc. If dest is blocked waiting for
     * the message, copy the message to it and unblock dest. Otherwise the caller
     * will be blocked and appended to the dest's sending queue.
     *
     * @param current  The caller, the sender.
     * @param dest     To whom the message is sent.
     * @param m        The message.
     *
     * @return Zero if success.
     *****************************************************************************/

    static int msg_send(Process* current, int dest, Message* m)
    {
        Process* sender = current;
        Process* p_dest = proc_table + dest; /* proc dest */

        assert(proc2pid(sender) != dest);

        /* check for deadlock here */
        if (deadlock(proc2pid(sender), dest)) {
            panic(">>DEADLOCK<< %s->%s", sender->name, p_dest->name);
        }

        if ((p_dest->state == RECEIVING) && /* dest is waiting for the msg */
            (p_dest->recvfrom == proc2pid(sender) ||
            p_dest->recvfrom == ANY)) {
            assert(p_dest->msg);
            assert(m);

            memcpy(va2la(dest, p_dest->msg),
                va2la(proc2pid(sender), m),
                sizeof(Message));
            p_dest->msg = 0;
            p_dest->state &= ~RECEIVING; /* dest has received the msg */
            p_dest->recvfrom = NO_TASK;
            unblock(p_dest);

            assert(p_dest->state == 0);
            assert(p_dest->msg == 0);
            assert(p_dest->recvfrom == NO_TASK);
            assert(p_dest->sendto == NO_TASK);
            assert(sender->state == 0);
            assert(sender->msg == 0);
            assert(sender->recvfrom == NO_TASK);
            assert(sender->sendto == NO_TASK);
        }
        else { /* dest is not waiting for the msg */
            sender->state |= SENDING;
            assert(sender->state == SENDING);
            sender->sendto = dest;
            sender->msg = m;

            /* append to the sending queue */
            Process * p;
            if (p_dest->q_sending) {
                p = p_dest->q_sending;
                while (p->next_sending)
                    p = p->next_sending;
                p->next_sending = sender;
            }
            else {
                p_dest->q_sending = sender;
            }
            sender->next_sending = 0;

            block(sender);

            assert(sender->state == SENDING);
            assert(sender->msg != 0);
            assert(sender->recvfrom == NO_TASK);
            assert(sender->sendto == dest);
        }

        return 0;
    }

    static int msg_receive(Process* current, int src, Message* m)
    {
        Process* p_who_wanna_recv = current; /**
                            * This name is a little bit
                            * wierd, but it makes me
                            * think clearly, so I keep
                            * it.
                            */
        Process* p_from = 0; /* from which the message will be fetched */
        Process* prev = 0;
        int copyok = 0;

        assert(proc2pid(p_who_wanna_recv) != src);

        if ((p_who_wanna_recv->has_int_msg) &&
            ((src == ANY) || (src == INTERRUPT))) {
            /* There is an interrupt needs p_who_wanna_recv's handling and
            * p_who_wanna_recv is ready to handle it.
            */

            Message msg;
            reset_msg(&msg);
            msg.source = INTERRUPT;
            msg.type = HARD_INT;

            assert(m);

            memcpy(va2la(proc2pid(p_who_wanna_recv), m), &msg,
                sizeof(Message));

            p_who_wanna_recv->has_int_msg = 0;

            assert(p_who_wanna_recv->state == 0);
            assert(p_who_wanna_recv->msg == 0);
            assert(p_who_wanna_recv->sendto == NO_TASK);
            assert(p_who_wanna_recv->has_int_msg == 0);

            return 0;
        }

        /* Arrives here if no interrupt for p_who_wanna_recv. */
        if (src == ANY) {
            /* p_who_wanna_recv is ready to receive messages from
            * ANY proc, we'll check the sending queue and pick the
            * first proc in it.
            */
            if (p_who_wanna_recv->q_sending) {
                p_from = p_who_wanna_recv->q_sending;
                copyok = 1;

                assert(p_who_wanna_recv->state == 0);
                assert(p_who_wanna_recv->msg == 0);
                assert(p_who_wanna_recv->recvfrom == NO_TASK);
                assert(p_who_wanna_recv->sendto == NO_TASK);
                assert(p_who_wanna_recv->q_sending != 0);
                assert(p_from->state == SENDING);
                assert(p_from->msg != 0);
                assert(p_from->recvfrom == NO_TASK);
                assert(p_from->sendto == proc2pid(p_who_wanna_recv));
            }
        }
        else if (src >= 0 && src < proc_num) {
            /* p_who_wanna_recv wants to receive a message from
            * a certain proc: src.
            */
            p_from = &proc_table[src];

            if ((p_from->state & SENDING) &&
                (p_from->sendto == proc2pid(p_who_wanna_recv))) {
                /* Perfect, src is sending a message to
                * p_who_wanna_recv.
                */
                copyok = 1;

                Process* p = p_who_wanna_recv->q_sending;

                assert(p); /* p_from must have been appended to the
                        * queue, so the queue must not be NULL
                        */

                while (p) {
                    assert(p_from->state & SENDING);

                    if (proc2pid(p) == src) /* if p is the one */
                        break;

                    prev = p;
                    p = p->next_sending;
                }

                assert(p_who_wanna_recv->state == 0);
                assert(p_who_wanna_recv->msg == 0);
                assert(p_who_wanna_recv->recvfrom == NO_TASK);
                assert(p_who_wanna_recv->sendto == NO_TASK);
                assert(p_who_wanna_recv->q_sending != 0);
                assert(p_from->state == SENDING);
                assert(p_from->msg != 0);
                assert(p_from->recvfrom == NO_TASK);
                assert(p_from->sendto == proc2pid(p_who_wanna_recv));
            }
        }

        if (copyok) {
            /* It's determined from which proc the message will
            * be copied. Note that this proc must have been
            * waiting for this moment in the queue, so we should
            * remove it from the queue.
            */
            if (p_from == p_who_wanna_recv->q_sending) { /* the 1st one */
                assert(prev == 0);
                p_who_wanna_recv->q_sending = p_from->next_sending;
                p_from->next_sending = 0;
            }
            else {
                assert(prev);
                prev->next_sending = p_from->next_sending;
                p_from->next_sending = 0;
            }

            assert(m);
            assert(p_from->msg);

            /* copy the message */
            memcpy(va2la(proc2pid(p_who_wanna_recv), m),
                va2la(proc2pid(p_from), p_from->msg),
                sizeof(Message));

            p_from->msg = 0;
            p_from->sendto = NO_TASK;
            p_from->state &= ~SENDING;

            unblock(p_from);
        }
        else {  /* nobody's sending any msg */
            /* Set state so that p_who_wanna_recv will not
            * be scheduled until it is unblocked.
            */
            p_who_wanna_recv->state |= RECEIVING;

            p_who_wanna_recv->msg = m;
            p_who_wanna_recv->recvfrom = src;
            block(p_who_wanna_recv);

            assert(p_who_wanna_recv->state == RECEIVING);
            assert(p_who_wanna_recv->msg != 0);
            assert(p_who_wanna_recv->recvfrom != NO_TASK);
            assert(p_who_wanna_recv->sendto == NO_TASK);
            assert(p_who_wanna_recv->has_int_msg == 0);
        }

        return 0;
    }

    int sys_sendrec(int function, int src_dest, Message* m, Process* p)
    {
        assert(k_reenter == 0);

        int ret = 0;
        int caller = proc2pid(p);
        Message* mla = (Message*)va2la(caller, m);
        mla->source = caller;

        assert(mla->source != src_dest);

        /**
         * Actually we have the third message type: BOTH. However, it is not
         * allowed to be passed to the kernel directly. Kernel doesn't know
         * it at all. It is transformed into a SEND followed by a RECEIVE
         * by `send_recv()'.
         */
        if (function == SEND) {
            ret = msg_send(p, src_dest, m);
            if (ret != 0)
                return ret;
        }
        else if (function == RECEIVE) {
            ret = msg_receive(p, src_dest, m);
            if (ret != 0)
                return ret;
        }
        else {
            panic("{sys_sendrec} invalid function: "
                "%d (SEND:%d, RECEIVE:%d).", function, SEND, RECEIVE);
        }

        return 0;
    }

    /*****************************************************************************
     *                                send_recv
     *****************************************************************************/
    /**
     * <Ring 1~3> IPC syscall.
     *
     * It is an encapsulation of `sendrec',
     * invoking `sendrec' directly should be avoided
     *
     * @param function  SEND, RECEIVE or BOTH
     * @param src_dest  The caller's proc_nr
     * @param msg       Pointer to the MESSAGE struct
     *
     * @return always 0.
     *****************************************************************************/
    int send_recv(int function, int src_dest, Message* msg)
    {
        int ret = 0;

        if (function == RECEIVE)
            memset(msg, 0, sizeof(Message));

        switch (function) {
        case BOTH:
            ret = sendrec(SEND, src_dest, msg);
            if (ret == 0)
                ret = sendrec(RECEIVE, src_dest, msg);
            break;
        case SEND:
        case RECEIVE:
            ret = sendrec(function, src_dest, msg);
            break;
        default:
            assert((function == BOTH) ||
                (function == SEND) || (function == RECEIVE));
            break;
        }

        return ret;
    }

    /*****************************************************************************
     *                                inform_int
     *****************************************************************************/
    /**
     * <Ring 0> Inform a proc that an interrupt has occured.
     *
     * @param task_nr  The task which will be informed.
     *****************************************************************************/
    void inform_int(int task_nr)
    {
        Process* p = proc_table + task_nr;

        if ((p->state & RECEIVING) && /* dest is waiting for the msg */
            ((p->recvfrom == INTERRUPT) || (p->recvfrom == ANY))) {
            p->msg->source = INTERRUPT;
            p->msg->type = HARD_INT;
            p->msg = 0;
            p->has_int_msg = 0;
            p->state &= ~RECEIVING; /* dest has received the msg */
            p->recvfrom = NO_TASK;
            assert(p->state == 0);
            unblock(p);

            assert(p->state == 0);
            assert(p->msg == 0);
            assert(p->recvfrom == NO_TASK);
            assert(p->sendto == NO_TASK);
        }
        else {
            p->has_int_msg = 1;
        }
    }
};




