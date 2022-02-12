/*
写主函数
*/

#include "bootpack.h"

struct BOOTINFO *const binfo = (struct BOOTINFO *)ADR_BOOTINFO;
extern struct FIFO8 keyfifo, mousefifo;

/*
内存分布
0x00000000 - 0x000fffff: 虽然在启动中会多次使用，但之后就变空。(1MB)
0x00100000 - 0x00267fff: 用于保存软盘的内容。(1440KB)
0x00268000 - 0x0026f7ff: 空 (30KB)
0x0026f800 - 0x0026ffff: IDT (2KB)
0x00270000 - 0x0027ffff: GDT (64KB)
0x00280000 - 0x002fffff: bootpack.hrb (512KB)
0x00300000 - 0x003fffff: 栈及其他 (1MB)
0x00400000 -           : 空
*/

unsigned int memtest(unsigned const int start, unsigned const int end);
unsigned int memtest_sub(unsigned int start, unsigned const int end);

#define MEMMAN_FREES 4090 // 内存管理表最大信息量
#define MEMMAN_ADDR 0x003c0000 // 内存管理表储存地址

struct FREEINFO
{ // 可用内存区的信息
    unsigned int addr, size;
};

struct MEMMAN
{ // 内存管理表
    // 可用信息数，可用信息总数，释放失败的内存大小总和，释放失败次数
    int frees, maxfrees, lostsize, losts;
    struct FREEINFO free[MEMMAN_FREES];
};

unsigned int memtest(unsigned const int start, unsigned const int end);
unsigned int memtest_sub(unsigned int start, unsigned const int end);
void memman_init(struct MEMMAN *const man);
unsigned int memman_total(struct MEMMAN const *const man);
unsigned int memman_alloc(struct MEMMAN *const man, unsigned const int size);
int memman_free(struct MEMMAN *const man, unsigned int const addr, unsigned int const size);


void HariMain(void)
{
    // 初始化中断
    init_gdtidt();
    init_pic();
    // 中断初始化完成，开放中断
    io_sti();

    // 键鼠FIFO初始化
    char keybuf[32];
    fifo8_init(&keyfifo, 32, keybuf);
    char mousebuf[128];
    fifo8_init(&mousefifo, 128, mousebuf);
    // 键鼠中断初始化
    io_out8(PIC0_IMR, 0xf9); // 11111001 开放PIC1和键盘中断
    io_out8(PIC1_IMR, 0xef); // 11101111 开放鼠标中断
    // 键盘控制器初始化
    init_keyboard();
    // 鼠标本体初始化
    struct MOUSE_DEC mdec;
    enable_mouse(&mdec);

    // 内存管理初始化
    unsigned int memtotal = memtest(0x00400000, 0xbfffffff); //总内存
    struct MEMMAN *const memman = (struct MEMMAN *const)MEMMAN_ADDR;
    memman_init(memman);
    memman_free(memman, 0x00001000, 0x0009e000);
    memman_free(memman, 0x00400000, memtotal - 0x00400000);

    // 初始化界面
    init_palette();
    init_screen8(binfo->VRAM, binfo->SCRNX, binfo->SCRNY);

    // 鼠标光标绘制
    char mcursor[256];
    init_mouse_cursor8(mcursor, lightdarkblue);
    int mx = (binfo->SCRNX - 16) / 2; //画面中心坐标
    int my = (binfo->SCRNY - 28 - 16) / 2;
    putblock8_8(binfo->VRAM, binfo->SCRNX, 16, 16, mx, my, mcursor, 16);

    // 鼠标坐标绘制
    char s[40];
    sprintf(s, "(%3d, %3d)", mx, my);
    putfonts8_asc(binfo->VRAM, binfo->SCRNX, 0, 0, white, s);

    // 内存信息显示
    sprintf(s, "memory %dMB free : %dKB", memtotal / (1024*1024), memman_total(memman) / 1024);
    putfonts8_asc(binfo->VRAM, binfo->SCRNX, 0, 32, white, s);

    int i;
    for (;;)
    {
        io_cli(); // 先屏蔽中断
        if (fifo8_status(&keyfifo) + fifo8_status(&mousefifo) == 0)
            /*
            此处使用stihlt指令，相当于STI指令后紧跟着HLT指令，
            这两条指令执行之间发生中断的话，中断会在HLT指令之后被受理。
            如果分开用sti和HLT，中间发生中断的话，
            CPU会进入HLT状态不处理中断。
            */
            io_stihlt(); // 恢复接受中断，待机
        else
        {
            if (fifo8_status(&keyfifo))
            {
                i = fifo8_get(&keyfifo);
                io_sti(); // 恢复中断
                sprintf(s, "%02X", i);
                boxfill8(binfo->VRAM, binfo->SCRNX, lightdarkblue, 0, 16, 15, 31);
                putfonts8_asc(binfo->VRAM, binfo->SCRNX, 0, 16, white, s);
            }
            else if (fifo8_status(&mousefifo))
            {
                i = fifo8_get(&mousefifo);
                io_sti();
                if (mouse_decode(&mdec, i))
                {
                    // 鼠标按键信息绘制
                    sprintf(s, "[lcr %4d %4d]", mdec.x, mdec.y);
                    if ((mdec.btn & 0x01) != 0)
                        s[1] = 'L';
                    if ((mdec.btn & 0x02) != 0)
                        s[3] = 'R';
                    if ((mdec.btn & 0x04) != 0)
                        s[2] = 'C';
                    boxfill8(binfo->VRAM, binfo->SCRNX, lightdarkblue, 32, 16, 32 + 15 * 8 - 1, 31);
                    putfonts8_asc(binfo->VRAM, binfo->SCRNX, 32, 16, white, s);
                    // 鼠标指针移动
                    boxfill8(binfo->VRAM, binfo->SCRNX, lightdarkblue, mx, my, mx + 15, my + 15);
                    mx += mdec.x;
                    my += mdec.y;
                    if (mx < 0)
                        mx = 0;
                    if (my < 0)
                        my = 0;
                    if (mx > binfo->SCRNX - 16)
                        mx = binfo->SCRNX - 16;
                    if (my > binfo->SCRNY - 16)
                        my = binfo->SCRNY - 16;
                    sprintf(s, "(%3d, %3d)", mx, my);
                    boxfill8(binfo->VRAM, binfo->SCRNX, lightdarkblue, 0, 0, 79, 15);
                    putfonts8_asc(binfo->VRAM, binfo->SCRNX, 0, 0, white, s);
                    putblock8_8(binfo->VRAM, binfo->SCRNX, 16, 16, mx, my, mcursor, 16);
                }
            }
        }
    }
}

#define EFLAGS_AC_BIT 0x00040000
#define CR0_CACHE_DISABLE 0x60000000

unsigned int memtest(unsigned const int start, unsigned const int end)
{
    /*
    内存测试。
    */

    /*
        原理：
    向内存中随便写入一个值，然后马上读取，如果内存连接正常，
    则写入的值能够存在内存中，读取的值与写入的相同；
    如果内存没连接，则读取不一定相同。
    注意，CPU缓存会将写入内存的值暂存，这样无论写哪里的内存，
    都能“正常”读出，影响内存测试，因此需要关闭缓存。
    */
    char flg486 = 0; // CPU是否为486以上
    unsigned int eflg = io_load_eflags();
    eflg |= EFLAGS_AC_BIT; // AC-bit := 1
    io_store_eflags(eflg);
    eflg = io_load_eflags();
    if (eflg & EFLAGS_AC_BIT) // 如果是386，就没有AC标志位，即使设置AC=1，AC还会自动回到1
        flg486 = 1;
    eflg &= ~EFLAGS_AC_BIT; // AC-bit := 0
    io_store_eflags(eflg);

    // 486以上CPU有缓存，缓存影响接下来的内存测试，要将其关闭
    if (flg486)
        store_cr0(load_cr0() | CR0_CACHE_DISABLE);

    // unsigned int i = asm_memtest_sub(start, end);
    unsigned int i = memtest_sub(start, end);

    if (flg486)
        store_cr0(load_cr0() & ~CR0_CACHE_DISABLE); // 打开缓存

    return i;
}

unsigned int memtest_sub(unsigned int start, unsigned const int end)
{
    /*
    内存测试核心过程。
    */
    unsigned int volatile *p = NULL;
    unsigned int old;
    unsigned const int pat0 = 0xaa55aa55, pat1 = 0x55aa55aa;
    for (; start <= end; start += 0x1000)
    {
        p = (unsigned int volatile *)(start + 0xffc); // 检查4KB内存的最后4字节
        old = *p;                                     // 保存以前的值
        *p = pat0;                                    // 试写
        *p ^= 0xffffffff;                             // 反转，防止内存缓存等
        if (*p != pat1)
        {
        not_memory:
            *p = old; // 恢复以前的值
            break;
        }
        *p ^= 0xffffffff; // 再次反转，同样为了防止内存缓存
        if (*p != pat0)
            goto not_memory;
        *p = old;
    }
    return start;
}

void memman_init(struct MEMMAN *const man)
{
    /*
    内存管理表初始化为空。
    */
    man->frees = 0;    /* 可用信息数目 */
    man->maxfrees = 0; /* 用于观察可用状况：frees的最大值 */
    man->lostsize = 0; /* 释放失败的内存的大小总和 */
    man->losts = 0;    /* 释放失败次数 */
    return;
}

unsigned int memman_total(struct MEMMAN const *const man)
{
    /*
    报告空余内存大小的总和。
    */
    unsigned int i, t = 0;
    for (i = 0; i < man->frees; i++)
        t += man->free[i].size;
    return t;
}

unsigned int memman_alloc(struct MEMMAN *const man, unsigned const int size)
{
    /*
    分配内存。
    返回非0值表示获取得到的内存空间首地址，
    返回0表示未获得合适的内存空间。
    */
    unsigned int i, a;
    for (i = 0; i < man->frees; i++)
        if (man->free[i].size >= size)
        {
            a = man->free[i].addr;
            man->free[i].addr += size;
            man->free[i].size -= size;
            if (man->free[i].size == 0)
            {
                man->frees--;
                for (; i < man->frees; i++)
                    man->free[i] = man->free[i + 1];
            }
        }
    return 0;
}

int memman_free(struct MEMMAN *const man, unsigned int const addr, unsigned int const size)
{
    /*
    释放内存空间。
    返回0表示成功,，-1表示失败。
    */

    int i, j;
    for (i = 0; i < man->frees; i++)
        if (man->free[i].addr > addr)
            break;
    /*
    free[]中的addr从小到大排序，所以现在有
    free[i - 1].addr < addr < free[i].addr
    */
    if (i > 0)
    { // addr前面存在可用内存块
        if (man->free[i - 1].addr + man->free[i - 1].size == addr)
        { // 与前面内存块恰好相连
            man->free[i - 1].size += size;
            if (i < man->frees)
            { // addr后面也存在可用内存块
                if (addr + size == man->free[i].addr)
                { // 与后面的内存块也刚好相连
                    man->free[i - 1].size += man->free[i].size;
                    man->frees--;
                    for (; i < man->frees; i++)
                        man->free[i] = man->free[i + 1];
                }
            }
            return 0;
        }
    }
    // 不能与前面的内存空间合并
    if (i < man->frees)
    { // addr后面存在可用内存块
        if (addr + size == man->free[i].addr)
        { // 与后面的内存块刚好相连
            man->free[i].addr = addr;
            man->free[i].size += size;
            return 0;
        }
    }
    // 前后都不能恰好相连
    if (man->frees < MEMMAN_FREES)
    {
        for (j = man->frees; j > i; i--)
            man->free[j] = man->free[j - 1];
        man->frees++;
        if (man->maxfrees < man->frees)
            man->maxfrees = man->frees;
        man->free[i].addr = addr;
        man->free[i].size = size;
        return 0;
    }
    // 不能往后移动
    man->losts++;
    man->lostsize += size;
    return -1;
}
