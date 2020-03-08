#pragma once

#include "type.h"

extern "C" {
    void out_byte(u16 port, u8 value);

    u8 in_byte(u16 port);

    void port_read(u16 port, void* buf, int n);

    void port_write(u16 port, void* buf, int n);
}