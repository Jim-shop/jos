/*
多任务
*/

#include "bootpack.h"

struct TIMER *task_timer; // 任务切换定时器
struct TASKCTL *taskctl;

struct TASK *task_now(void)
{
    /*
    获得正在运行的task。
    */
    struct TASKLEVEL *tl = &taskctl->level[taskctl->now_lv];
    return tl->tasks[tl->now];
}

void task_add(struct TASK *const task)
{
    /*
    往任务本身所设置的tasklevel中加入task。
    */
    struct TASKLEVEL *tl = &taskctl->level[task->level];
    tl->tasks[tl->running] = task;
    tl->running++;
    // 小心：有超界风险
    task->flags = TASK_FLAGS_RUNNING;
    return;
}

void task_remove(struct TASK *const task)
{
    /*
    从任务所属的tasklevel中删除task。
    */
    struct TASKLEVEL *tl = &taskctl->level[task->level];
    // 找到位于活动tasklevel中的序号。
    int i;
    for (i = 0; i < tl->running; i++)
        if (tl->tasks[i] == task)
            break;
    tl->running--;
    if (i < tl->now)
        tl->now--;
    if (tl->now >= tl->running)
        tl->now = 0;
    task->flags = TASK_FLAGS_SLEEP;
    for (; i < tl->running; i++) // 移位
        tl->tasks[i] = tl->tasks[i + 1];
    return;
}

void task_switchsub(void)
{
    /*
    在任务切换时决定接下来切换到哪个level。
    */
    int i;
    for (i = 0; i < MAX_TASKLEVELS; i++)
        if (taskctl->level[i].running > 0) // 靠前的等级先运行
            break;
    taskctl->now_lv = i;
    taskctl->lv_change = 0; // 设定为不切换其他lv。需要切换时由其他函数指定
    return;
}

struct TASK *task_init(struct MEMMAN *const memman)
{
    /*
    初始化任务管理表，返回一个任务指针。
    */
    taskctl = (struct TASKCTL *)memman_alloc_4k(memman, sizeof(struct TASKCTL));
    int i;
    for (i = 0; i < MAX_TASKS; i++)
    {
        taskctl->tasks0[i].flags = 0;
        taskctl->tasks0[i].sel = (TASK_GDT0 + i) * 8;
        set_segmdesc(gdt + TASK_GDT0 + i, 103, (int)&taskctl->tasks0[i].tss, AR_TSS32);
    }
    for (i = 0; i < MAX_TASKLEVELS; i++)
    {
        taskctl->level[i].running = 0;
        taskctl->level[i].now = 0;
    }
    struct TASK *task = task_alloc();
    task->flags = TASK_FLAGS_RUNNING;
    task->priority = 2; // 0.02s
    task->level = 0;
    task_add(task);
    task_switchsub(); // 初始化LEVEL
    load_tr(task->sel);
    task_timer = timer_alloc();
    // 没有fifo，不必timer_init()
    timer_settime(task_timer, task->priority);
    return task;
}

struct TASK *task_alloc(void)
{
    /*
    分配一个任务。返回一个任务指针。如果分配失败则返回空指针。
    */
    int i;
    struct TASK *task;
    for (i = 0; i < MAX_TASKS; i++)
    {
        if (taskctl->tasks0[i].flags == 0)
        {
            task = &taskctl->tasks0[i];
            task->flags = TASK_FLAGS_SLEEP; // 使用中
            task->tss.eflags = 0x00000202;  // 中断允许标志IF=1
            task->tss.eax = 0;              // 这里先设为0
            task->tss.ecx = 0;
            task->tss.edx = 0;
            task->tss.ebx = 0;
            task->tss.ebp = 0;
            task->tss.esi = 0;
            task->tss.edi = 0;
            task->tss.es = 0;
            task->tss.ds = 0;
            task->tss.fs = 0;
            task->tss.gs = 0;
            task->tss.ldtr = 0;
            task->tss.iomap = 0x40000000;
            return task;
        }
    }
    return NULL;
}

void task_run(struct TASK *const task, int level, const int priority)
{
    /*
    将一个任务(执行或未执行)标记为执行状态并设定优先级。
    */
    if (level < 0) // 设为非正数表示不改变LEVEL
        level = task->level;
    if (priority > 0) // 设为非整数表示不改变优先级
        task->priority = priority;

    if (task->flags == TASK_FLAGS_RUNNING && task->level != level) // 改变活动中的LEVEL
        task_remove(task);                                         // 从LEVEL中删除，顺便设置SLEEP，于是下面的也会执行
    if (task->flags != TASK_FLAGS_RUNNING)
    { // 从休眠状态唤醒
        task->flags = TASK_FLAGS_RUNNING;
        task_add(task);
    }
    taskctl->lv_change = 1; // 下次切换时检查LEVEL
    return;
}

void task_switch(void)
{
    /*
    由定时器触发，在运行的任务之间切换。
    */
    struct TASKLEVEL *tl = &taskctl->level[taskctl->now_lv];
    struct TASK *now_task = tl->tasks[tl->now];
    tl->now++;
    if (tl->now == tl->running)
        tl->now = 0; 
    if (taskctl->lv_change) // 如果先前因为进程变动（休眠、增加、减少）而需要检查LEVEL
    {
        task_switchsub();
        tl = &taskctl->level[taskctl->now_lv];
    }
    struct TASK *new_task = tl->tasks[tl->now];
    timer_settime(task_timer, new_task->priority);
    if(new_task!=now_task)
        farjmp(0, new_task->sel);
    return;
}

void task_sleep(struct TASK *const task)
{
    /*
    将一个进程标记为休眠。
    由于本函数中没有屏蔽中断，所以进入此函数前要屏蔽中断，
    不然可能出问题。
    */
    if (task->flags == TASK_FLAGS_RUNNING)
    {
        struct TASK *now_task = task_now();
        task_remove(task);
        if (task == now_task)
        { // task是running且tasklevel是running，则需要执行进程切换
            task_switchsub();
            now_task = task_now();
            farjmp(0, now_task->sel);
        }
    }
    return;
}
