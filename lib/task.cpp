#include "task.h"

#include "proc.h"
#include "string.h"

namespace task {
    u32 sys_task_num;

    u32 user_task_num;

    Task   sys_tasks[kMaxSysTask];

    Task   usr_tasks[kMaxUsrTask];

    void add_sys_task(task_f initial_eip, int stacksize, const char* name)
    {
        u32 i = sys_task_num;
        sys_tasks[i].initial_eip = initial_eip;
        sys_tasks[i].stacksize = stacksize;
        strcpy(sys_tasks[i].name, name);
        sys_task_num++;

        proc::add_proc(sys_tasks + i, 1);
    }

    void add_user_task(task_f initial_eip, int stacksize, const char* name)
    {
        u32 i = user_task_num;
        usr_tasks[i].initial_eip = initial_eip;
        usr_tasks[i].stacksize = stacksize;
        strcpy(usr_tasks[i].name, name);
        user_task_num++;

        proc::add_proc(usr_tasks + i, 3);
    }

    void init_task()
    {
        sys_task_num = 0;
        user_task_num = 0;
    }
};

