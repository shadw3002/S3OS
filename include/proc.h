#pragma once

#include "type.h"
#include "protect.h"
#include "task.h"

namespace proc {
    struct Process;

    namespace ipc {
        enum Type {
            SEND,
            RECEIVE,
            BOTH
        };

        enum MessageType {
            HARD_INT = 1,

            /* SYS task */
            GET_TICKS,
            GET_CHAR,

            /* message type for drivers */
            DEV_OPEN = 1001,
            DEV_CLOSE,
            DEV_READ,
            DEV_WRITE,
            DEV_IOCTL
        };

        struct Message{
            struct mess1 {
                int m1i1;
                int m1i2;
                int m1i3;
                int m1i4;
            };
            struct mess2 {
                void* m2p1;
                void* m2p2;
                void* m2p3;
                void* m2p4;
            };
            struct mess3 {
                int    m3i1;
                int    m3i2;
                int    m3i3;
                int    m3i4;
                u64    m3l1;
                u64    m3l2;
                void*    m3p1;
                void*    m3p2;
            };

            int source;
            int type;
            union {
                struct mess1 m1;
                struct mess2 m2;
                struct mess3 m3;
            } u;
        };

        int sys_sendrec(int function, int src_dest, Message* m, Process* p);
        int send_recv(int function, int src_dest, Message* msg);
        void reset_msg(Message* msg);
        void inform_int(int task_nr);
    };

    constexpr u32 kProcStackSize = 0x40000;

    extern u32 used_proc_stack;

    extern char proc_stack[kProcStackSize];

    void add_proc(task::Task* p_task, u32 ring);
    void init_proc_table();
    void schedule();
    void* va2la(int pid, void* va);
    void clone(Process* p);

    constexpr u32 kMaxProc  = 16;

    constexpr i32 INTERRUPT    = -10;

    enum ProcState {
        READY,
        SENDING,
        RECEIVING,
        NEW
    };

    enum ProcType {
        ANY = kMaxProc,
        NO_TASK
    };

    struct StackFrame {    /* proc_ptr points here     ↑ Low            */
        u32    gs;            /* ┓                        │            */
        u32    fs;            /* ┃                        │            */
        u32    es;            /* ┃                        │            */
        u32    ds;            /* ┃                        │            */
        u32    edi;           /* ┃                        │            */
        u32    esi;           /* ┣ pushed by save()       │            */
        u32    ebp;           /* ┃                        │            */
        u32    kernel_esp;    /* <- 'popad' will ignore it │ */
        u32    ebx;           /* ┃                        ↑栈从高地址往低地址增长*/
        u32    edx;           /* ┃                        │            */
        u32    ecx;           /* ┃                        │
              */
        u32    eax;           /* ┛                        │            */
        u32    retaddr;    /* return address for assembly code save()    │            */
        u32    eip;           /*  ┓                        │            */
        u32    cs;            /*  ┃                        │            */
        u32    eflags;        /*  ┣ these are pushed by CPU during interrupt    │            */
        u32    esp;           /*  ┃                        │            */
        u32    ss;            /*  ┛                        ┷High            */
    };

    enum Priority {
        N1,
        N2,
        N3,
        N4,
        N5
    };


    struct Process {
        StackFrame regs;          /* process registers saved in stack frame */

        u16 ldt_sel;               /* gdt selector giving ldt base and limit */
        Descriptor ldts[LDT_SIZE]; /* local descriptors for code and data */

            int ticks;                 /* remained ticks */
            int priority;

        u32 pid;                   /* process id passed in from MM */
        char name[16];           /* name of the process */

        u32 state;

        ipc::Message* msg;
        u32 recvfrom;
        u32 sendto;

        int has_int_msg;           /**
                        * nonzero if an INTERRUPT occurred when
                        * the task is not ready to deal with it.
                        */

        Process* q_sending;   /**
                        * queue of procs sending messages to
                        * this proc
                        */
        Process* next_sending;/**
                        * next proc in the sending
                        * queue (q_sending)
                        */

        int nr_tty;

        u32 stack_size;

        char* stack;

        bool is_father;

        u32 thread_list;
    };

    extern u32 proc_num;
    extern Process    proc_table[kMaxProc];


};

#define    RETVAL        u.m3.m3i1

extern Tss tss;
extern proc::Process*   p_proc_ready;



extern "C" {
    void restart();
}

void TestA();
void TestB();
void shell_main();



