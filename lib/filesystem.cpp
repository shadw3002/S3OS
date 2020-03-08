#include "filesystem.h"

#include "hd_driver.h"
#include "stdio.h"
#include "proc.h"
#include "assert.h"

using namespace proc;

static char buf[20480];

void task_fs()
{
    printf("[FileSystem]\n");
    proc::ipc::Message driver_msg;
    //driver_msg.type = DEV_OPEN;
    //send_recv(BOTH, 2, &driver_msg);




    spin("FS");
}