#pragma once

#include "proc.h"
#include "type.h"

constexpr u32 NR_SYS_CALL = 5;

extern system_call	sys_call_table[NR_SYS_CALL];

int     sys_write(u32 u1, u32 u2, char* buf, proc::Process* p_proc);
u8      sys_getchar(u32 u1, u32 u2, u32 u3, proc::Process* p_proc);
void    sys_fork(u32 u1, u32 u2, u32 u3, proc::Process* p_proc);
u32     sys_get_pid(u32 u1, u32 u2, u32 u3, proc::Process* p_proc);


extern "C" {
    void    write(const char* buf);
    u8      getch();
    u32     sendrec(int function, int src_dest, proc::ipc::Message* msg);
    void    fork();
    u32     get_pid();
}