#include "type.h"
#include "proc.h"

void    sys_fork(u32 u1, u32 u2, u32 u3, proc::Process* p_proc);

u32     sys_get_pid(u32 u1, u32 u2, u32 u3, proc::Process* p_proc);