#include "systask.h"

#include "assert.h"
#include "proc.h"
#include "clock.h"

using namespace proc;

void task_sys()
{
    proc::ipc::Message msg;
    while (1) {
        send_recv(proc::ipc::Type::RECEIVE, ANY, &msg);
        int src = msg.source;
        switch (msg.type) {
        case proc::ipc::GET_TICKS:
            msg.RETVAL = ticks;
            send_recv(proc::ipc::Type::SEND, src, &msg);
            break;
        default:
            panic("unknow msg type");
            break;
        }
    }


}