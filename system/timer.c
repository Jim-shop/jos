/*
控制可编程间隔型定时器 PIT。
Intel 8254可编程定时器芯片 1193181 Hz
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
    int i;
    for (i = 0; i < MAX_TIMER; i++)
        timerctl.timers0[i].flags = TIMER_FLAGS_FREE;
    struct TIMER *t = timer_alloc(); // 哨兵
    t->timeout = 0xffffffff;
    t->flags = TIMER_FLAGS_USING;
    t->next = 0; // 末尾
    timerctl.t0 = t;
    timerctl.next = 0xffffffff;
    return;
}

struct TIMER *timer_alloc(void)
{
    /*
    获得一个timer的地址。如果所有timer都被分配完，就返回NULL指针。
    */
    int i;
    for (i = 0; i < MAX_TIMER; i++)
        if (timerctl.timers0[i].flags == TIMER_FLAGS_FREE)
        {
            timerctl.timers0[i].flags = TIMER_FLAGS_ALLOC;
            timerctl.timers0[i].flags2 = 0;
            return &timerctl.timers0[i];
        }
    return NULL;
}

void timer_free(struct TIMER *const timer)
{
    /*
    释放一个timer。
    */
    timer->flags = TIMER_FLAGS_FREE; // 未使用
    return;
}

void timer_init(struct TIMER *const timer, struct FIFO32 *const fifo, unsigned int const data)
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
    设定timer的定时。timeout: 相对时间
    由于有了哨兵机制，新加入的timer不会是最后一个timer。
    */
    timer->timeout = timeout + timerctl.count; // 转化为绝对时间
    timer->flags = TIMER_FLAGS_USING;
    int eflags = io_load_eflags();
    io_cli(); // 时钟中断会影响这里的设置，先关了
    struct TIMER *t = timerctl.t0;
    if (timer->timeout <= t->timeout)
    { // 插入最前面
        timer->next = t;
        timerctl.t0 = timer;
        timerctl.next = timer->timeout;
        io_store_eflags(eflags);
        return;
    }
    // 插不到最前面
    struct TIMER *s;
    for (;;)
    {
        s = t;
        t = t->next;
        if (timer->timeout <= t->timeout)
        { // 插到s,t之间
            s->next = timer;
            timer->next = t;
            io_store_eflags(eflags);
            return;
        }
    }
    // 除非哨兵出问题，否则应该到不了这里
}

int timer_cancel(struct TIMER *const timer)
{
    /*
    取消定时器定时。
    返回1表示处理成功，0表示不需要处理。
    */
    struct TIMER *t;
    int eflags = io_load_eflags();
    io_cli();
    if (timer->flags == TIMER_FLAGS_USING)
    {
        if (timer == timerctl.t0) // 如果是第一个定时器
        {
            t = timer->next;
            timerctl.t0 = t;
            timerctl.next = t->timeout;
        }
        else // 如果不是第一个，就要找到前一个定时器
        {
            t = timerctl.t0;
            for (;;)
            {
                if (t->next == timer)
                    break;
                t = t->next;
            }
            t->next = timer->next;
        }
        timer->flags = TIMER_FLAGS_ALLOC;
        io_store_eflags(eflags);
        return 1; // 成功
    }
    io_store_eflags(eflags);
    return 0; // 不需要取消处理
}

void timer_cancelall(struct FIFO32 const *const fifo)
{
    /*
    取消以fifo为缓冲区的所有具有自动取消属性的timer。
    */
    struct TIMER *t;
    int eflags = io_load_eflags();
    io_cli();
    int i;
    for (i = 0; i < MAX_TIMER; i++)
    {
        t = &timerctl.timers0[i];
        if (t->flags != TIMER_FLAGS_FREE && t->flags2 != 0 && t->fifo == fifo)
        {
            timer_cancel(t);
            timer_free(t);
        }
    }
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
    // 能执行到这行以下的说明有定时器超时：
    char ts = 0;
    struct TIMER *timer = timerctl.t0;
    for (;;)
    {
        if (timer->timeout > timerctl.count)
            break;
        timer->flags = TIMER_FLAGS_ALLOC;
        if (timer != task_timer)
            fifo32_put(timer->fifo, timer->data);
        else
            ts = 1;
        timer = timer->next;
    }
    timerctl.t0 = timer;
    timerctl.next = timer->timeout; // 有哨兵兜底，不怕越界
    if (ts)
        task_switch(); // timer全部处理完成后再切换任务
    return;
}
