#pragma once

#include "type.h"

namespace task {
    constexpr u32 kMaxSysTask = 8;
    constexpr u32 kMaxUsrTask = 8;


    struct Task {
        task_f    initial_eip;
        int    stacksize;
        char    name[32];
    };

    extern u32 sys_task_num;

    extern u32 user_task_num;

    extern Task       sys_tasks[kMaxSysTask];
    
    extern Task       usr_tasks[kMaxUsrTask];

    void init_task();

    void add_sys_task(task_f initial_eip, int stacksize, const char* name);

    void add_user_task(task_f initial_eip, int stacksize, const char* name);
};