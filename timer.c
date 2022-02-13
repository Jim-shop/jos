/*
控制可编程间隔型定时器 PIT。
1193180 Hz
PIT 连接在 IRQ 0。
*/

#include "bootpack.h"

struct TIMERCTL timerctl;

void init_pit(void)
{
    /*
    设置中断周期（三步）：
        1. AL := 0x34；OUT(0x43, AL);
        2. AL := 中断周期的低8位; OUT(0x40, AL);
        3. AL := 中断周期的高8位; OUT(0x40, AL);
    如果中断周期设成0，会被当做指定为 65536。
    中断周期实际上是每隔多少个主频发送一次中断信号。
    */
    io_out8(PIT_CTRL, 0x34);
    io_out8(PIT_CNT0, 0x9c);
    io_out8(PIT_CNT0, 0x2e); // 0x2e9c = 11932，这样大约是100Hz
    timerctl.count = 0;
    timerctl.next = 0xffffffff;
    timerctl.using = 0;
    int i;
    for (i = 0; i < MAX_TIMER; i++)
        timerctl.timers0[i].flags = 0;
    return;
}

struct TIMER *timer_alloc(void)
{
    /*
    获得一个timer的地址。如果所有timer都被分配完，就返回NULL指针。
    */
    int i;
    for (i = 0; i < MAX_TIMER; i++)
        if (timerctl.timers0[i].flags == 0)
        {
            timerctl.timers0[i].flags = TIMER_FLAGS_ALLOC;
            return &timerctl.timers0[i];
        }
    return NULL;
}

void timer_free(struct TIMER *const timer)
{
    /*
    释放一个timer。
    */
    timer->flags = 0; // 未使用
    return;
}

void timer_init(struct TIMER *const timer, struct FIFO8 *const fifo, unsigned char const data)
{
    /*
    初始化timer。
    */
    timer->fifo = fifo;
    timer->data = data;
    return;
}

void timer_settime(struct TIMER *const timer, unsigned int const timeout)
{
    /*
    设定timer的定时。
    */
    timer->timeout = timeout + timerctl.count;
    timer->flags = TIMER_FLAGS_USING;
    int eflags = io_load_eflags();
    io_cli(); // 时钟中断会影响这里的设置，先关了
    int i;
    // 找到插入位置
    for (i = 0; i < timerctl.using; i++)
        if (timerctl.timers[i]->timeout >= timer->timeout)
            break;
    // 后面的后移
    int j;
    for (j = timerctl.using; j > i; j--)
        timerctl.timers[j] = timerctl.timers[j - 1];
    // 插入
    timerctl.using ++;
    timerctl.timers[i] = timer;
    timerctl.next = timerctl.timers[0]->timeout;
    io_store_eflags(eflags);
    return;
}

void inthandler20(int *esp)
{
    /*
    处理时钟中断。IRQ 0
    */
    io_out8(PIC0_OCW2, 0x60); // 中断处理完毕
    timerctl.count++;
    if (timerctl.next > timerctl.count)
        return;
    // 有定时器超时：
    int i;
    for (i = 0; i < timerctl.using; i++)
    {
        if (timerctl.timers[i]->timeout > timerctl.count)
            break;
        timerctl.timers[i]->flags = TIMER_FLAGS_ALLOC;
        fifo8_put(timerctl.timers[i]->fifo, timerctl.timers[i]->data);
    }
    // 有i个定时器超时
    timerctl.using -= i;
    int j;
    for (j = 0; j < timerctl.using; j++)
        timerctl.timers[j] = timerctl.timers[i + j];
    if (timerctl.using > 0)
        timerctl.next = timerctl.timers[0]->timeout;
    else
        timerctl.next = 0xffffffff;

    return;
}
