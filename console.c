/*
提供控制台任务。
*/

#include "bootpack.h"

// 文件信息存在软盘0x2600处
struct FILEINFO *const finfo = (struct FILEINFO *const)(ADR_DISKIMG + 0x002600);

void console_task(struct SHEET *const sheet, unsigned int const memtotal)
{
    /*
    控制台进程。
    */

    // 自身task
    struct TASK *task = task_now();

    // 事件队列
    int fifobuf[128];
    fifo32_init(&task->fifo, 128, fifobuf, task);

    // 光标
    struct CONSOLE cons;
    cons.sht = sheet;
    cons.cur_x = 8;
    cons.cur_y = 28;
    cons.cur_c = -1;
    struct TIMER *timer = timer_alloc();
    timer_init(timer, &task->fifo, 1);
    timer_settime(timer, 50);

    // 暂存指令
    char cmdline[30];

    // fat表 0柱面0磁头2~9扇区第一份10~18第二份（两份完全相同）
    unsigned short *const fat = (unsigned short *const)memman_alloc_4k(memman, 2 * 2880); // 共2880扇区
    file_readfat(fat, (unsigned char *)(ADR_DISKIMG + 0x000200));

    // 传递cons给API接口
    *((int *)0x0fec) = (int)&cons;

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
                    timer_init(timer, &task->fifo, 0);
                    if (cons.cur_c >= 0)
                        cons.cur_c = white;
                }
                else
                {
                    timer_init(timer, &task->fifo, 1);
                    if (cons.cur_c >= 0)
                        cons.cur_c = black;
                }
                timer_settime(timer, 50);
                break;

            case 2: // 光标ON
                cons.cur_c = white;
                break;

            case 3: // 光标OFF
                boxfill8(sheet->buf, sheet->bxsize, black, cons.cur_x, cons.cur_y, cons.cur_x + 7, cons.cur_y + 15);
                cons.cur_c = -1;
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
    if (strcmp(cmdline, "mem") == 0)
        cmd_mem(cons, memtotal);
    else if (strcmp(cmdline, "cls") == 0)
        cmd_cls(cons);
    else if (strcmp(cmdline, "dir") == 0)
        cmd_dir(cons);
    else if (strncmp(cmdline, "type ", 5) == 0)
        cmd_type(cons, fat, cmdline);
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
        1003*8: 应用程序代码段
        1004*8: 应用程序数据段
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
            char *q = (char *)memman_alloc_4k(memman, seg_size);                  // 应用程序数据段
            *((int *)0xfe8) = (int)q;                                             // 传递段号给API
            set_segmdesc(gdt + 1003, f->size - 1, (int)p, AR_CODE32_ER + 0x60);   // 应用程序权限
            set_segmdesc(gdt + 1004, 64 * 1024 - 1, (int)q, AR_DATA32_RW + 0x60); // 应用程序权限
            for (i = 0; i < data_size; i++)
                q[esp + i] = p[data_je + i];
            start_app(0x1b, 1003 * 8, esp, 1004 * 8, &(task_now()->tss.esp0)); // 从主函数开始执行
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
    6. 窗口中叠写字符串（EBX=窗口句柄，(ESI,EDI)=(x,y)，EAX=色号，ECX=字符串长度，EBP=字符串）
    7. 窗口中绘制方块（EBX=窗口句柄，(EAX,ECX)=(x0,y0)，(ESI,EDI)=(x1,y1)不超尾，EBP=色号）
    */
    int ds_base = *((int *)0xfe8);
    struct TASK *task = task_now();
    struct CONSOLE *const cons = (struct CONSOLE *const)*((int *)0x0fec);
    struct SHTCTL *const shtctl = (struct SHTCTL *const)*((int *)0x0fe4);
    struct SHEET *sht;
    int *reg = &eax + 1; // eax的后一个位置，即第一次PUSHAD储存的位置。修改它达到返回值效果
    /*
    reg[0] : EDI,   reg[1] : ESI,   reg[2] : EBP,   reg[3] : ESP
    reg[4] : EBX,   reg[5] : EDX,   reg[6] : ECX,   reg[7] : EAX
    */
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
        sheet_setbuf(sht, (char *)ebx + ds_base, esi, edi, eax);
        make_window8((char *)ebx + ds_base, esi, edi, (char *)ecx + ds_base, 0);
        sheet_slide(sht, 100, 50);
        sheet_updown(sht, 3);
        reg[7] = (int)sht;
        break;

    case 6:
        sht = (struct SHEET *)ebx;
        putfonts8_asc(sht->buf, sht->bxsize, esi, edi, eax, (char *)ebp + ds_base);
        sheet_refresh(sht, esi, edi, esi + ecx * 8, edi + 16);
        break;

    case 7:
        sht = (struct SHEET *)ebx;
        boxfill8(sht->buf, sht->bxsize, ebp, eax, ecx, esi, edi);
        sheet_refresh(sht, eax, ecx, esi + 1, edi + 1); // 超尾
        break;
    }
    return 0; // 正常返回
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
    struct CONSOLE *const cons = (struct CONSOLE *const)*((int *)0x0fec);
    cons_putstr0(cons, "\nINT 0C:\nStack Exception.\n");
    sprintf(s, "EIP = %08X\n", esp[11]); // 栈的第11号元素（EIP）
    cons_putstr0(cons, s);
    return &(task_now()->tss.esp0); // 强制结束程序
}

int *inthandler0d(int *esp)
{
    /*
    处理INT 0d: 应用程序越权中断。
    强制结束程序并打印错误提醒。
    */
    char s[30];
    struct CONSOLE *const cons = (struct CONSOLE *const)*((int *)0x0fec);
    cons_putstr0(cons, "\nINT 0D:\nGeneral Protected Exception.\n");
    sprintf(s, "EIP = %08X\n", esp[11]); // 栈的第11号元素（EIP）
    cons_putstr0(cons, s);
    return &(task_now()->tss.esp0); // 强制结束程序
}
