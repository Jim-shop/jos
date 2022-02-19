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

                case '\n' + 256:                   // 回车信号
                    cons_putchar(&cons, ' ', 0);   // 擦除光标
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
    else if (strcmp(cmdline, "hlt") == 0)
        cmd_hlt(cons, fat);
    else if (cmdline[0] != 0) // 不是命令也不是空行
    {
        putfonts8_asc_sht(cons->sht, 8, cons->cur_y, white, black, "Wrong instruction.", 18);
        cons_newline(cons);
        cons_newline(cons);
    }
    return;
}

void cmd_mem(struct CONSOLE *const cons, unsigned int const memtotal)
{
    /*
    给控制台提供mem命令。
    指令功能：查询总计内存和剩余内存
    */
    char s[30];
    sprintf(s, "total   %dMB", memtotal / (1024 * 1024));
    putfonts8_asc_sht(cons->sht, 8, cons->cur_y, white, black, s, 30);
    cons_newline(cons);
    sprintf(s, "free %dKB", memman_total(memman) / 1024);
    putfonts8_asc_sht(cons->sht, 8, cons->cur_y, white, black, s, 30);
    cons_newline(cons);
    cons_newline(cons);
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
                sprintf(s, "filename.ext   %7d", finfo[x].size);
                for (y = 0; y < 8; y++)
                    s[y] = finfo[x].name[y]; // 文件名
                s[9] = finfo[x].ext[0];
                s[10] = finfo[x].ext[1];
                s[11] = finfo[x].ext[2]; // 扩展名
                putfonts8_asc_sht(cons->sht, 8, cons->cur_y, white, black, s, 30);
                cons_newline(cons);
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
    int y;
    if (f != NULL) // 找到了
    {
        char *const p = (char *)memman_alloc_4k(memman, f->size);
        file_loadfile(f->clustno, f->size, p, fat, (unsigned char *)(ADR_DISKIMG + 0x003e00));
        for (y = 0; y < f->size; y++)
            cons_putchar(cons, p[y], 1);
        memman_free_4k(memman, (unsigned int)p, f->size);
    }
    else // 没找到
    {
        putfonts8_asc_sht(cons->sht, 8, cons->cur_y, white, black, "File not found.", 15);
        cons_newline(cons);
    }
    cons_newline(cons);
    return;
}

void cmd_hlt(struct CONSOLE *const cons, unsigned short const *const fat)
{
    /*
    启动 hlt.je
    */
    struct FILEINFO *f = file_search("HLT.JE", finfo, 224);
    if (f != NULL) // 找到了
    {
        char *const p = (char *)memman_alloc_4k(memman, f->size);
        file_loadfile(f->clustno, f->size, p, fat, (unsigned char *)(ADR_DISKIMG + 0x003e00));
        set_segmdesc(gdt + 1003, f->size - 1, (int)p, AR_CODE32_ER);
        farjmp(0, 1003 * 8);
        memman_free_4k(memman, (unsigned int)p, f->size);
    }
    else // 没找到
    {
        putfonts8_asc_sht(cons->sht, 8, cons->cur_y, white, black, "File not found.", 15);
        cons_newline(cons);
    }
    cons_newline(cons);
    return;
}
