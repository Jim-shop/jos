/*
提供控制台任务。
*/

#include "bootpack.h"

// 文件信息存在软盘0x2600处
struct FILEINFO *const finfo = (struct FILEINFO *const)(ADR_DISKIMG + 0x002600);

// 各个任务主函数不要使用return语句，因为return返回[ESP]地址，而[ESP]这里没有写入
void console_task(struct SHEET *const sheet, unsigned int const memtotal)
{
    /*
    控制台进程。
    */

    // 自身task
    struct TASK *task = task_now();

    // 状态信息
    struct CONSOLE cons;
    cons.sht = sheet;
    cons.cur_x = 8;
    cons.cur_y = 28;
    cons.cur_c = -1;
    if (sheet != NULL)
    {
        cons.timer = timer_alloc();
        timer_init(cons.timer, &task->fifo, 1);
        timer_settime(cons.timer, 50);
    }
    task->cons = &cons;

    // 暂存指令
    char cmdline[30];

    // fat表 0柱面0磁头2~9扇区第一份10~18第二份（两份完全相同）
    unsigned short *const fat = (unsigned short *const)memman_alloc_4k(memman, 2 * 2880); // 共2880扇区
    file_readfat(fat, (unsigned char *)(ADR_DISKIMG + 0x000200));

    // 提示符
    cons_putchar(&cons, '>', 1);

    int i;
    for (;;)
    {
        io_cli();
        if (fifo32_status(&task->fifo) == 0)
        {
            task_sleep(task);
            io_sti();
        }
        else
        {
            i = fifo32_get(&task->fifo);
            io_sti();
            switch (i)
            {
            case 0 ... 1: // 光标闪烁
                if (i != 0)
                {
                    timer_init(cons.timer, &task->fifo, 0);
                    if (cons.cur_c >= 0)
                        cons.cur_c = white;
                }
                else
                {
                    timer_init(cons.timer, &task->fifo, 1);
                    if (cons.cur_c >= 0)
                        cons.cur_c = black;
                }
                timer_settime(cons.timer, 50);
                break;

            case 2: // 光标ON
                cons.cur_c = white;
                break;

            case 3: // 光标OFF
                boxfill8(sheet->buf, sheet->bxsize, black, cons.cur_x, cons.cur_y, cons.cur_x + 7, cons.cur_y + 15);
                cons.cur_c = -1;
                break;

            case 4: // 退出
                cmd_exit(&cons, fat);
                break;

            case 256 ... 511: // 键盘数据（从任务A得到）
                switch (i)
                {
                case '\b' + 256: // 退格信号
                    if (cons.cur_x > 16)
                    {
                        cons_putchar(&cons, ' ', 0);
                        cons.cur_x -= 8;
                    }
                    break;

                case '\n' + 256:                     // 回车信号
                    cons_putchar(&cons, ' ', 0);     // 擦除光标
                    cmdline[cons.cur_x / 8 - 2] = 0; // 字串结束标志
                    cons_newline(&cons);
                    cons_runcmd(cmdline, &cons, fat, memtotal);
                    if (sheet == NULL)
                        cmd_exit(&cons, fat);
                    cons_putchar(&cons, '>', 1); // 显示提示符
                    break;

                default: // 可显示字符
                    if (cons.cur_x < 240)
                    {
                        cmdline[cons.cur_x / 8 - 2] = i - 256; // 读入
                        cons_putchar(&cons, i - 256, 1);
                    }
                    break;
                }
                break;
            }
            // 光标
            if (cons.cur_c >= 0)
                boxfill8(sheet->buf, sheet->bxsize, cons.cur_c, cons.cur_x, cons.cur_y, cons.cur_x + 7, cons.cur_y + 15);
            sheet_refresh(sheet, cons.cur_x, cons.cur_y, cons.cur_x + 8, cons.cur_y + 16);
        }
    }
}

void cons_newline(struct CONSOLE *const cons)
{
    /*
    给控制台换行并回车。行满时滚动。
    使用前记得把光标擦除。
    */
    if (cons->sht != NULL)
    {
        int x, y;
        if (cons->cur_y < 28 + 112) // 没填满所有行
            cons->cur_y += 16;      // 换行
        else                        // 填满了所有行
        {
            for (y = 28; y < 28 + 112; y++)
                for (x = 8; x < 8 + 240; x++)
                    cons->sht->buf[x + y * cons->sht->bxsize] = cons->sht->buf[x + (y + 16) * cons->sht->bxsize]; // 滚动
            for (y = 28 + 112; y < 28 + 128; y++)
                for (x = 8; x < 8 + 240; x++)
                    cons->sht->buf[x + y * cons->sht->bxsize] = black; // 清空
            sheet_refresh(cons->sht, 8, 28, 8 + 240, 28 + 128);
        }
    }
    cons->cur_x = 8;
    return;
}

void cons_putchar(struct CONSOLE *const cons, const char ch, const char move)
{
    /*
    向控制台打印一个字符。
    move: (bool)控制打印字符后光标是否移动。
    */
    switch (ch)
    {
    case '\t':
        do
        {
            if (cons->sht != NULL)
                putfont8_sht(cons->sht, cons->cur_x, cons->cur_y, white, black, ' ');
            cons->cur_x += 8;
            if (cons->cur_x == 8 + 240)
                cons_newline(cons);
        } while ((cons->cur_x - 8) & 0x1f); // 输出空格，直到cons->cur_x被32整除（即四个空格对齐）
        break;

    case '\n':
        cons_newline(cons);
        break;

    case '\r':
        break; // 忽略

    default:
        if (cons->sht != NULL)
            putfont8_sht(cons->sht, cons->cur_x, cons->cur_y, white, black, ch);
        if (move)
        {
            cons->cur_x += 8;
            if (cons->cur_x == 8 + 240)
                cons_newline(cons);
        }
        break;
    }
    return;
}

void cons_putstr0(struct CONSOLE *const cons, char const *s)
{
    /*
    向控制台中打印一个字符串，遇'\0'停止。
    */
    for (; *s != 0; s++)
        cons_putchar(cons, *s, 1);
    return;
}

void cons_putstr1(struct CONSOLE *const cons, char const *const s, const int l)
{
    /*
    向控制台中打印 l 位长度的字符串。
    */
    int i;
    for (i = 0; i < l; i++)
        cons_putchar(cons, s[i], 1);
    return;
}

void cons_runcmd(char const *const cmdline, struct CONSOLE *const cons, unsigned short const *const fat, unsigned int const memtotal)
{
    /*
    识别控制台输入的指令，调用相应处理函数或程序。
    */
    if (strcmp(cmdline, "mem") == 0 && cons->sht != NULL)
        cmd_mem(cons, memtotal);
    else if (strcmp(cmdline, "cls") == 0 && cons->sht != NULL)
        cmd_cls(cons);
    else if (strcmp(cmdline, "dir") == 0 && cons->sht != NULL)
        cmd_dir(cons);
    else if (strncmp(cmdline, "type ", 5) == 0 && cons->sht != NULL)
        cmd_type(cons, fat, cmdline);
    else if (strcmp(cmdline, "exit") == 0)
        cmd_exit(cons, fat);
    else if (strncmp(cmdline, "start ", 6) == 0)
        cmd_start(cons, cmdline, memtotal);
    else if (strncmp(cmdline, "ncst ", 5) == 0)
        cmd_ncst(cons, cmdline, memtotal);
    else if (cmdline[0] != 0)                 // 不是内置命令也不是空行
        if (cmd_app(cons, fat, cmdline) == 0) // 也不是程序
            cons_putstr0(cons, "Wrong instruction or application name.\n\n");
    return;
}

int cmd_app(struct CONSOLE *const cons, unsigned short const *const fat, char const *const cmdline)
{
    /*
    查找以cmdline为文件名的je程序执行。找不到返回0，执行成功返回1
    */
    struct TASK *task = task_now();
    char name[18];
    int i;
    // 生成文件名
    for (i = 0; i < 13; i++)
    {
        if (cmdline[i] <= ' ')
            break;
        name[i] = cmdline[i];
    }
    name[i] = 0;
    struct FILEINFO *f = file_search(name, finfo, 224);
    if (f == NULL && name[i - 1] != '.')
    { // 找不到文件，就在文件名后加上".je"重新找
        name[i] = '.';
        name[i + 1] = 'J';
        name[i + 2] = 'E';
        name[i + 3] = 0;
        f = file_search(name, finfo, 224);
    }
    if (f != NULL) // 找到了
    {
        /*
            段使用情况：
        1*8: 操作系统数据段
        2*8: 操作系统代码段
        3*8~1002*8: TSS
        1003*8~2002*8: 应用程序代码段
        2003*8~3002*8: 应用程序数据段
        */
        char *const p = (char *)memman_alloc_4k(memman, f->size); // 应用程序代码段
        file_loadfile(f->clustno, f->size, p, fat, (char *)(ADR_DISKIMG + 0x3e00));
        /*
            .hrb文件格式
        0x0000 (DWORD)      请求操作系统为应用程序准备的数据段大小
        0x0004 (DWORD)      "Hari"
        0x0008 (DWORD)      数据段内预备空间的大小
        0x000c (DWORD)      ESP初始值 & 数据部分传送目的地址
        0x0010 (DWORD)      hrb文件内数据部分的大小
        0x0014 (DWORD)      hrb文件内数据部分从哪里开始
        0x0018 (DWORD)      0xe9000000 (0x1b位置连同后面0x001c的内容构成跳转到入口的机器指令)
        0x001c (DRORD)      应用程序运行入口地址减0x20
        0x0020 (DWORD)      ma11oc空间的起始地址
        */
        if (f->size >= 36 && strncmp(p + 4, "Hari", 4) == 0 && *p == 0x00) // 段大小4k对齐，低8位一定为0
        {
            int seg_size = *((int *)(p + 0x0000));
            int esp = *((int *)(p + 0x000c));
            int data_size = *((int *)(p + 0x0010));
            int data_je = *((int *)(p + 0x0014));
            char *q = (char *)memman_alloc_4k(memman, seg_size);                                 // 应用程序数据段
            task->ds_base = (int)q;                                                              // 传递段号给API
            set_segmdesc(gdt + 1000 + task->sel / 8, f->size - 1, (int)p, AR_CODE32_ER + 0x60);  // 应用程序权限
            set_segmdesc(gdt + 2000 + task->sel / 8, seg_size - 1, (int)q, AR_DATA32_RW + 0x60); // 应用程序权限
            for (i = 0; i < data_size; i++)
                q[esp + i] = p[data_je + i];
            start_app(0x1b, 1000 * 8 + task->sel, esp, 2000 * 8 + task->sel, &(task->tss.esp0)); // 从主函数开始执行
            struct SHTCTL *shtctl = (struct SHTCTL *)*((int *)0x0fe4);
            struct SHEET *sht;
            for (i = 0; i < MAX_SHEETS; i++)
            {
                sht = &(shtctl->sheets0[i]);
                if ((sht->flags & 0x11) == 0x11 && sht->task == task) // 有应用程序运行0x10且SHT_FLAGS_USING
                    sheet_free(sht);                                  // 释放被应用程序遗留的窗口
            }
            timer_cancelall(&task->fifo); // 取消以此fifo为缓冲区的所有具有自动取消属性的timer
            memman_free_4k(memman, (unsigned int)q, seg_size);
        }
        else
            cons_putstr0(cons, ".je file format error.\n");
        memman_free_4k(memman, (unsigned int)p, f->size);
        cons_newline(cons);
        return 1;
    }
    return 0; // 没找到
}

void cmd_mem(struct CONSOLE *const cons, unsigned int const memtotal)
{
    /*
    给控制台提供mem命令。
    指令功能：查询总计内存和剩余内存
    */
    char s[60];
    sprintf(s, "total   %dMB\nfree %dKB\n\n", memtotal / (1024 * 1024), memman_total(memman) / 1024);
    cons_putstr0(cons, s);
    return;
}

void cmd_cls(struct CONSOLE *const cons)
{
    /*
    给控制台提供cls命令。
    指令功能：清屏。(请自行处理提示符)
    */
    int x, y;
    for (y = 28; y < 28 + 128; y++)
        for (x = 8; x < 8 + 240; x++)
            cons->sht->buf[x + y * cons->sht->bxsize] = black;
    sheet_refresh(cons->sht, 8, 28, 8 + 240, 28 + 128);
    cons->cur_y = 28;
    return;
}

void cmd_dir(struct CONSOLE *const cons)
{
    /*
    给控制台提供dir命令。
    指令功能：显示文件清单。
    */

    int x, y;
    char s[30];
    for (x = 0; x < 224; x++) // 文件表中最多224项文件
    {
        if (finfo[x].name[0] == 0x00) // 没有文件信息了
            break;
        if (finfo[x].name[0] != 0xe5) // 不是已删除的文件
        {
            if ((finfo[x].type & 0x18) == 0) // 不是目录或非文件信息
            {
                sprintf(s, "filename.ext   %7d\n", finfo[x].size);
                for (y = 0; y < 8; y++)
                    s[y] = finfo[x].name[y]; // 文件名
                s[9] = finfo[x].ext[0];
                s[10] = finfo[x].ext[1];
                s[11] = finfo[x].ext[2]; // 扩展名
                cons_putstr0(cons, s);
            }
        }
    }
    cons_newline(cons);
    return;
}

void cmd_type(struct CONSOLE *const cons, unsigned short const *const fat, char const *const cmdline)
{
    /*
    给控制台提供type命令。
    指令功能：根据type命令后跟的文件名（不区分大小写）找到对应的文件并显示内容。
    */

    struct FILEINFO const *const f = file_search(cmdline + 5, finfo, 224);
    if (f != NULL) // 找到了
    {
        char *const p = (char *)memman_alloc_4k(memman, f->size);
        file_loadfile(f->clustno, f->size, p, fat, (unsigned char *)(ADR_DISKIMG + 0x003e00));
        cons_putstr1(cons, p, f->size);
        memman_free_4k(memman, (unsigned int)p, f->size);
    }
    else // 没找到
        cons_putstr0(cons, "File not found.\n");
    cons_newline(cons);
    return;
}

void cmd_exit(struct CONSOLE const *const cons, short const *const fat)
{
    /*
    给控制台提供exit指令。
    指令功能：释放内存空间，通知task_a()进一步处理。
    */
    struct TASK *task = task_now();
    struct SHTCTL *shtctl = (struct SHTCTL *)*((int *)0x0fe4);
    struct FIFO32 *fifo = (struct FIFO32 *)*((int *)0x0fec); // task_a的
    if (cons->sht != NULL)
        timer_cancel(cons->timer);
    memman_free_4k(memman, (int)fat, 4 * 2880);
    io_cli();
    if (cons->sht != NULL)
        fifo32_put(fifo, cons->sht - shtctl->sheets0 + 768); // 768~1023
    else
        fifo32_put(fifo, task - taskctl->tasks0 + 1024); // 1024~2023
    io_sti();
    for (;;)
        task_sleep(task);
}

void cmd_start(struct CONSOLE *const cons, char const *cmdline, const int memtotal)
{
    /*
    给控制台提供start指令。
    指令功能：另起一个控制台，传递start后面的指令。
    */
    struct SHTCTL *shtctl = (struct SHTCTL *)*((int *)0x0fe4);
    struct SHEET *sht = open_console(shtctl, memtotal);
    struct FIFO32 *fifo = &sht->task->fifo;
    sheet_slide(sht, 32, 4);
    sheet_updown(sht, shtctl->top);
    for (cmdline += 6; *cmdline != 0; cmdline++)
        fifo32_put(fifo, *cmdline + 256);
    fifo32_put(fifo, '\n' + 256);
    cons_newline(cons);
    return;
}

void cmd_ncst(struct CONSOLE *const cons, char const *cmdline, const int memtotal)
{
    /*
    给控制台提供ncst指令（no console start）。
    指令功能：另起一个控制台，传递start后面的指令，但不显示控制台。
    */
    struct TASK *task = open_constask(NULL, memtotal);
    struct FIFO32 *fifo = &task->fifo;
    for (cmdline += 5; *cmdline != 0; cmdline++)
        fifo32_put(fifo, *cmdline + 256);
    fifo32_put(fifo, '\n' + 256);
    cons_newline(cons);
    return;
}

int *je_api(int edi, int esi, int ebp, int esp, int ebx, int edx, int ecx, int eax)
{
    /*
    对接实现汇编api。
        汇编api调用方法：
    1. EDX 填入功能号
    2. INT 0x40
        功能号表：
    1. 显示单个字符（AL=字符编码）
    2. 显示以'\0'结尾字符串（EBX=字符串地址）
    3. 显示某长度字符串（EBX=字符串地址，ECX=字符串长度）
    4. 结束应用程序
    5. 显示窗口（EBX=窗口缓冲区，ESI=窗口x大小，EDI=窗口y大小，EAX=透明色，ECX=窗口名称）->EAX=窗口句柄
    6. 窗口中叠写字符串（EBX=窗口句柄(最低bit为0则刷新窗口)，(ESI,EDI)=(x,y)，EAX=色号，ECX=字符串长度，EBP=字符串）
    7. 窗口中绘制方块（EBX=窗口句柄(最低bit为0则刷新窗口)，(EAX,ECX)=(x0,y0)，(ESI,EDI)=(x1,y1)不超尾，EBP=色号）
    8. memman初始化（EBX=memman地址，EAX=空间地址，ECX=空间大小）
    9. malloc（EBX=memman地址，ECX=请求字节数）->EAX=分配到的内存地址
    10. free（EBX=memman地址，EAX=释放的地址，ECX=释放字节数）
    11. 窗口中画点（EBX=窗口句柄(最低bit为0则刷新窗口)，(ESI,EDI)=(x,y)，EAX=色号）
    12. 刷新窗口（EBX=窗口句柄，(EAX,ECX)=(x0,y0)，(ESI,EDI)=(x1,y1)超尾）
    13. 窗口中画直线（EBX=窗口句柄(最低bit为0则刷新窗口)，(EAX,ECX)=(x0,y0)，(ESI,EDI)=(x1,y1)不超尾，EBP=色号）
    14. 关闭窗口（EBX=窗口句柄）
    15. 获取键盘/定时器输入（EAX={0:不阻塞，没有输入时返回-1; 1:阻塞}）->EAX=输入的字符编码
    16. 获取定时器alloc->EAX=定时器句柄
    17. 设置定时器的发送数据init（EBX=定时器句柄，EAX=数据）
    18. 定时器时间设定set（EBX=定时器句柄，EAX=时间）
    19. 释放定时器free（EBX=定时器句柄）
    20. 蜂鸣器发声（EAX=声音频率mHz,0表示停止发声）
    */
    struct TASK *task = task_now();
    int ds_base = task->ds_base;
    struct CONSOLE *const cons = task->cons;
    struct SHTCTL *const shtctl = (struct SHTCTL *const)*((int *)0x0fe4);
    int *reg = &eax + 1; // eax的后一个位置，即第一次PUSHAD储存的位置。修改它达到返回值效果
    /*
    reg[0] : EDI,   reg[1] : ESI,   reg[2] : EBP,   reg[3] : ESP
    reg[4] : EBX,   reg[5] : EDX,   reg[6] : ECX,   reg[7] : EAX
    */

    struct SHEET *sht;
    int i;
    switch (edx)
    {
    case 1:
        cons_putchar(cons, eax & 0xff, 1);
        break;

    case 2:
        cons_putstr0(cons, (char *)ebx + ds_base);
        break;

    case 3:
        cons_putstr1(cons, (char *)ebx + ds_base, ecx);
        break;

    case 4:
        return &(task->tss.esp0);
        break;

    case 5:
        sht = sheet_alloc(shtctl);
        sht->task = task;
        sht->flags |= 0x10; // 是应用程序窗口
        sheet_setbuf(sht, (char *)ebx + ds_base, esi, edi, eax);
        make_window8((char *)ebx + ds_base, esi, edi, (char *)ecx + ds_base, 0);
        sheet_slide(sht, ((shtctl->xsize - esi) / 2) & ~3, (shtctl->ysize - edi) / 2); // 居中，x方向对齐到4，加快运算
        sheet_updown(sht, shtctl->top);                                                // 移动到顶层。鼠标自动上移
        reg[7] = (int)sht;
        break;

    case 6:
        sht = (struct SHEET *)(ebx & 0xfffffffe); // 忽略EBX最低bit
        putfonts8_asc(sht->buf, sht->bxsize, esi, edi, eax, (char *)ebp + ds_base);
        if ((ebx & 1) == 0) // EBX最低bit为0则刷新窗口
            sheet_refresh(sht, esi, edi, esi + ecx * 8, edi + 16);
        break;

    case 7:
        sht = (struct SHEET *)(ebx & 0xfffffffe); // 忽略EBX最低bit
        boxfill8(sht->buf, sht->bxsize, ebp, eax, ecx, esi, edi);
        if ((ebx & 1) == 0)                                 // EBX最低bit为0则刷新窗口
            sheet_refresh(sht, eax, ecx, esi + 1, edi + 1); // 超尾
        break;

    case 8:
        memman_init((struct MEMMAN *)(ebx + ds_base));
        ecx &= 0xfffffff0; // 16字节为单位
        memman_free((struct MEMMAN *)(ebx + ds_base), eax, ecx);
        break;

    case 9:
        ecx = (ecx + 0x0f) & 0xfffffff0; // 上取整到16字节
        reg[7] = memman_alloc((struct MEMMAN *)(ebx + ds_base), ecx);
        break;

    case 10:
        ecx = (ecx + 0x0f) & 0xfffffff0; // 上取整到16字节
        memman_free((struct MEMMAN *)(ebx + ds_base), eax, ecx);
        break;

    case 11:
        sht = (struct SHEET *)(ebx & 0xfffffffe); // 忽略EBX最低bit
        sht->buf[sht->bxsize * edi + esi] = eax;
        if ((ebx & 1) == 0)                                 // EBX 最低bit为0则刷新窗口
            sheet_refresh(sht, esi, edi, esi + 1, edi + 1); // 超尾
        break;

    case 12:
        sht = (struct SHEET *)ebx;
        sheet_refresh(sht, eax, ecx, esi, edi); // 超尾
        break;

    case 13:
        sht = (struct SHEET *)(ebx & 0xfffffffe); // 忽略EBX最低bit
        je_api_linewin(sht, eax, ecx, esi, edi, ebp);
        if ((ebx & 1) == 0)                                 // EBX最低bit为0则刷新窗口
            sheet_refresh(sht, eax, ecx, esi + 1, edi + 1); // 超尾
        break;

    case 14:
        sheet_free((struct SHEET *)ebx);
        break;

    case 15:
        for (;;)
        {
            io_cli();
            if (fifo32_status(&task->fifo) == 0)
            {
                if (eax) // 阻塞模式
                    task_sleep(task);
                else
                {
                    io_sti();
                    reg[7] = -1;
                    return 0;
                }
            }
            i = fifo32_get(&task->fifo);
            io_sti();
            switch (i)
            {
            case 0 ... 1: // 光标用定时器
                // 应用程序运行时不需要光标，所以不需要闪
                timer_init(cons->timer, &task->fifo, 1);
                timer_settime(cons->timer, 50);
                break;

            case 2: // 光标ON
                cons->cur_c = white;
                break;

            case 3: // 光标OFF
                cons->cur_c = -1;
                break;

            default:          // taskA传过来的键盘数据、应用程序自行设置的定时器数据
                if (i >= 256) // 应用程序数据+256=主调函数数据
                {
                    reg[7] = i - 256;
                    return 0;
                }
                break;
            }
        }
        break;

    case 16:
        reg[7] = (int)timer_alloc();
        ((struct TIMER *)reg[7])->flags2 = 1; // 允许自动取消
        break;

    case 17:
        timer_init((struct TIMER *)ebx, &task->fifo, eax + 256); // 应用程序数据+256=主调函数数据
        break;

    case 18:
        timer_settime((struct TIMER *)ebx, eax);
        break;

    case 19:
        timer_free((struct TIMER *)ebx);
        break;

    case 20:
        /*
        蜂鸣器由PIT控制。
            音高操作：
        1. AL=0xb6, OUT(0x43, AL)
        2. AL=设定值低8位, OUT(0x42, AL)
        3. AL=设定值高8位, OUT(0x42, AL)
        -> 设定值为0当作65536.
        -> 发生的音高为PIT时钟频率(1193181Hz)除以设定值
            蜂鸣器开关控制：
        ON:  IN(AL, 0x61), AL|=0x03, AL&=0x0f, OUT(0x61, AL)
        OFF: IN(AL, 0x61), AL&=0x0d, OUT(0x61, AL)
        */
        if (eax == 0) // 关闭蜂鸣器
            io_out8(0x61, io_in8(0x61) & 0x0d);
        else
        {
            i = 1193181000 / eax;
            io_out8(0x43, 0xb6);
            io_out8(0x42, i & 0xff);
            io_out8(0x42, i >> 8);
            io_out8(0x61, (io_in8(0x61) | 0x03) & 0x0f);
        }
    }
    return 0; // 正常返回
}

void je_api_linewin(struct SHEET *const sht, const int x0, const int y0, const int x1, const int y1, const int col)
{
    /*
    je_api()子模块。在窗口上画线。精度为1/1024
    */
    int dx = x1 - x0;
    int dy = y1 - y0;
    int x = x0 << 10;
    int y = y0 << 10;
    if (dx < 0)
        dx = -dx;
    if (dy < 0)
        dy = -dy; // x,y方向总增量
    int len;      // 将x,y方向中变化较大的作为len
    if (dx >= dy) // x,y方向步进
    {
        len = dx + 1; // 防止线长度为0时画不出点
        if (x0 <= x1)
            dx = 1024;
        else
            dx = -1024;
        if (y0 <= y1)
            dy = ((y1 - y0 + 1) << 10) / len;
        else
            dy = ((y1 - y0 - 1) << 10) / len;
    }
    else
    {
        len = dy + 1;
        if (y0 <= y1)
            dy = 1024;
        else
            dy = -1024;
        if (x0 <= x1)
            dx = ((x1 - x0 + 1) << 10) / len;
        else
            dx = ((x1 - x0 - 1) << 10) / len;
    }
    int i;
    for (i = 0; i < len; i++)
    {
        sht->buf[(y >> 10) * sht->bxsize + (x >> 10)] = col;
        x += dx;
        y += dy;
    }
}

/*
触发异常时寄存器值：
esp[0]: EDI
esp[1]: ESI
esp[2]: EBP
esp[4]: EBX
esp[5]: EDX
esp[6]: ECX
esp[7]: EAX
    esp[0...7]为_asm_inthandler中PUSHAD的结果
esp[8]: DS
esp[9]: ES
    esp[8...9]为_asm_inthandler中PUSH的结果
esp[10]: 错误编号（基本上是0，没什么收集价值）
esp[11]: EIP
esp[12]: CS
esp[13]: EFLAGS
esp[14]: ESP（应用程序用）
esp[15]: SS（应用程序用）
    esp[10...15]为异常产生时CPU自动PUSH的结果
*/

int *inthandler0c(int *esp)
{
    /*
    处理INT 0c: 栈异常。
    强制结束程序并打印错误提醒。
    */
    char s[30];
    struct TASK *const task = task_now();
    struct CONSOLE *const cons = task->cons;
    cons_putstr0(cons, "\nINT 0C:\nStack Exception.\n");
    sprintf(s, "EIP = %08X\n", esp[11]); // 栈的第11号元素（EIP）
    cons_putstr0(cons, s);
    return &(task->tss.esp0); // 强制结束程序
}

int *inthandler0d(int *esp)
{
    /*
    处理INT 0d: 应用程序越权中断。
    强制结束程序并打印错误提醒。
    */
    char s[30];
    struct TASK *const task = task_now();
    struct CONSOLE *const cons = task->cons;
    cons_putstr0(cons, "\nINT 0D:\nGeneral Protected Exception.\n");
    sprintf(s, "EIP = %08X\n", esp[11]); // 栈的第11号元素（EIP）
    cons_putstr0(cons, s);
    return &(task->tss.esp0); // 强制结束程序
}
