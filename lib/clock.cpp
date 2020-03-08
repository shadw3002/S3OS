
#include "type.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "interruption.h"
#include "port.h"
#include "clock.h"
#include "syscall.h"
#include "proc.h"

int		ticks;

namespace clock {
    void clock_handler(int irq)
    {
        ticks++;
        p_proc_ready->ticks--;

        if (k_reenter != 0) {
            return;
        }

        if (p_proc_ready->ticks > 0) {
            return;
        }

        proc::schedule();
    }

    void milli_delay(int milli_sec)
    {
        int t = get_ticks();

        while(((get_ticks() - t) * 1000 / HZ) < milli_sec) {}
    }

    void init()
    {
        ticks = 0;

        /* 初始化 8253 PIT */
        out_byte(TIMER_MODE, RATE_GENERATOR);
        out_byte(TIMER0, (u8) (TIMER_FREQ/HZ) );
        out_byte(TIMER0, (u8) ((TIMER_FREQ/HZ) >> 8));

        put_irq_handler(CLOCK_IRQ, clock_handler);    /* 设定时钟中断处理程序 */
        enable_irq(CLOCK_IRQ);                        /* 让8259A可以接收时钟中断 */
    }


    int get_ticks()
    {
        proc::ipc::Message msg;
        reset_msg(&msg);
        msg.type = proc::ipc::MessageType::GET_TICKS;
        send_recv(proc::ipc::Type::BOTH, 0, &msg);
        return msg.RETVAL;
    }
}