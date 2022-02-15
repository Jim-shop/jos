/*
多任务
*/

#include "bootpack.h"

struct TIMER *mt_timer; // 多任务切换
int mt_tr;              // 上一个任务段（段号*8）

void mt_init(void)
{
    mt_timer = timer_alloc();
    // 没有fifo，不必timer_init()
    timer_settime(mt_timer, 2); // 0.02s
    mt_tr = 3 * 8;
    return;
}

void mt_taskswitch(void)
{
    if (mt_tr == 3 * 8)
        mt_tr = 4 * 8;
    else
        mt_tr = 3 * 8;
    timer_settime(mt_timer, 2);
    farjmp(0, mt_tr);
    return;
}
