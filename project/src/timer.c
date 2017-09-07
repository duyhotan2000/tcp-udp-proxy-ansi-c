#include "timer.h"

int64_t counter = 0;

uv_prepare_t prep;
uv_check_t check;
uv_idle_t idler;

void timer_run(uv_loop_t* loop)
{
    uv_check_init(loop, &check);
    uv_check_start(&check, timer_check_cb);

    uv_idle_init(loop, &idler);
    uv_idle_start(&idler, idle_cb);
}

void timer_check_cb(uv_check_t *handle)
{
    if(++counter == 10000000)
    {
        printf("Timer still working\n");
        counter =0;
    }
//    if(now() = 8:50)
//    {
//        STATE = GET_SC;
//    }
}

void idle_cb(uv_idle_t *handle)
{
    //Low priority job
    //Do nothing
}
