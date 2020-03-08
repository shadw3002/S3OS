#pragma once

extern int		ticks;

namespace clock {
    void clock_handler(int irq);

    void init();

    void milli_delay(int milli_sec);

    int get_ticks();
}