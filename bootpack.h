#ifndef __bootpack_h_
#define __bootpack_h_

// asmhead 中先前存的信息
#define ADR_BOOTINFO 0x00000ff0
struct BOOTINFO
{                                // 位于0x0ff0~0x0fff
    unsigned char CYLS;          // 读入的柱面数
    unsigned char LEDS;          // 键盘LED状态
    unsigned char VMODE;         // 颜色位数
    unsigned char _RESERVE;      // 结构体对齐
    unsigned short SCRNX, SCRNY; // 屏幕分辨率
    unsigned char *VRAM;         // 图像缓冲区开始地址
};

// naskfunc 提供的函数
void io_hlt(void);
void io_cli(void);
void io_sti(void);
void io_stihlt(void);
int io_in8(const int port);
void io_out8(const int port, const int data);
int io_load_eflags(void);
void io_store_eflags(const int eflags);
void load_gdtr(const int limit, const int addr);
void load_idtr(const int limit, const int addr);
int load_cr0(void);
void store_cr0(const int cr0);
void asm_inthandler21(void);
void asm_inthandler27(void);
void asm_inthandler2c(void);
unsigned int asm_memtest_sub(unsigned int start, unsigned int end); //已用C语言实现，仅做备份

// font 提供的字库
extern const char font[4096];

// GOlib 头文件
#include <stdio.h>

// graphic.c
void init_palette(void);
void set_palette(int start, const int end, unsigned char *rgb);
void boxfill8(unsigned char *const vram, const int xsize, unsigned char const c, int const x0, int y0, int const x1, int const y1);
void init_screen8(unsigned char *const vram, int const x, int const y);
void putfont8(unsigned char *const vram, int const xsize, int const x, int const y, char const c, char const *const font);
void putfonts8_asc(unsigned char *const vram, const int xsize, int x, const int y, const char c, unsigned char const *s);
void init_mouse_cursor8(char *const mouse, const char bc);
void putblock8_8(unsigned char *const vram, const int vxsize, const int pxsize, const int pysize, const int px0, const int py0, char const *const buf, const int bxsize);
enum COLOR
{                       // 默认调色板颜色
    black = 0,          // 000000 黑
    red = 1,            // ff0000 亮红
    green = 2,          // 00ff00 亮绿
    yellow = 3,         // ffff00 亮黄
    blue = 4,           // 0000ff 亮蓝
    purple = 5,         // ff00ff 亮紫
    lightblue = 6,      // 00ffff 浅亮蓝
    white = 7,          // ffffff 白
    gray = 8,           // c6c6c6 亮灰
    darkred = 9,        // 840000 暗红
    darkgreen = 10,     // 008400 暗绿
    darkyellow = 11,    // 848400 暗黄
    darkblue = 12,      // 000084 暗青
    darkpurple = 13,    // 840084 暗紫
    lightdarkblue = 14, // 008484 浅暗蓝
    darkgray = 15       // 848484 暗灰
};

// dsctbl.c
struct SEGMENT_DESCRIPTOR
{ // GDT中的8字节内容
    short limit_low, base_low;
    char base_mid, access_right;
    char limit_high, base_high;
};
struct GATE_DESCRIPTOR
{ // IDT中的8字节内容
    short offset_low, selector;
    char dw_count, access_right;
    short offset_high;
};
void init_gdtidt(void);
void set_segmdesc(struct SEGMENT_DESCRIPTOR *const sd, unsigned int limit, const int base, int ar);
void set_gatedesc(struct GATE_DESCRIPTOR *const gd, const int offset, const int selector, const int ar);
#define ADR_IDT 0x0026f800   // 任选的，储存IDT表的位置
#define LIMIT_IDT 0x000007ff // IDT表的项数
#define ADR_GDT 0x00270000   // 任选的，储存GDT表的位置
#define LIMIT_GDT 0x0000ffff // GDT表的项数
#define ADR_BOTPAK 0x00280000
#define LIMIT_BOTPAK 0x0007ffff
#define AR_DATA32_RW 0x4092
#define AR_CODE32_ER 0x409a
#define AR_INTGATE32 0x008e // IDT属性，表示用于中断处理的有效设定

// int.c
void init_pic(void);
void inthandler27(int *esp);
#define PIC0_ICW1 0x0020
#define PIC0_OCW2 0x0020
#define PIC0_IMR 0x0021 // 中断屏蔽寄存器，8位对应8路IRQ信号，哪位为1屏蔽哪位
#define PIC0_ICW2 0x0021
#define PIC0_ICW3 0x0021
#define PIC0_ICW4 0x0021
#define PIC1_ICW1 0x00a0
#define PIC1_OCW2 0x00a0
#define PIC1_IMR 0x00a1 // 中断屏蔽寄存器，8位对应8路IRQ信号，哪位为1屏蔽哪位
#define PIC1_ICW2 0x00a1
#define PIC1_ICW3 0x00a1
#define PIC1_ICW4 0x00a1
#define PORT_KEYDAT 0x0060

// fifo.c
struct FIFO8
{
    unsigned char *buf;
    // 队尾，队头，缓冲区总大小，缓冲区空闲数，标志位
    unsigned int end, start, size, free, flags;
};
void fifo8_init(struct FIFO8 *const fifo, const int size, unsigned char *const buf);
int fifo8_put(struct FIFO8 *const fifo, unsigned char const data);
int fifo8_get(struct FIFO8 *const fifo);
int fifo8_status(struct FIFO8 *const fifo);
#define FLAGS_OVERRUN 0x0001

// keyboard.c
void wait_KBC_sendready(void);
void init_keyboard(void);
void inthandler21(int *esp);
#define PORT_KEYDAT 0x0060        // 键盘输入的数据（编码）
#define PORT_KEYSTA 0x0064        // 键盘控制电路的状态输出设备号
#define PORT_KEYCMD 0x0064        // 键盘控制电路的指令设置设备号
#define KEYSTA_SEND_NOTREADY 0x02 // KBC状态输出数据的掩码，得到倒数第二位的值
#define KEYCMD_WRITE_MODE 0x60    // 进入设定模式的指令
#define KBC_MODE 0x47             // 鼠标模式

// mouse.c
struct MOUSE_DEC
{
    /*
    除了初始化时产生的0xfa，鼠标发送的数据是3字节1组
    */
    // phase: 0等待0xfa, 1,2,3表示等待第几字节
    unsigned char buf[3], phase;
    int x, y, btn;
};
void enable_mouse(struct MOUSE_DEC *const mdec);
int mouse_decode(struct MOUSE_DEC *const mdec, unsigned char const dat);
void inthandler2c(int *esp);
#define KEYCMD_SENDTO_MOUSE 0xd4 // 向键盘控制电路发送这个数据，写往DAT的下一个数据就会自动转发给鼠标
#define MOUSECMD_ENABLE 0xf4     // 激活鼠标指令

// memory.c
#define EFLAGS_AC_BIT 0x00040000
#define CR0_CACHE_DISABLE 0x60000000
#define MEMMAN_FREES 4090      // 内存管理表最大信息量
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
unsigned int memman_alloc_4k(struct MEMMAN *const man, unsigned int const size);
int memman_free_4k(struct MEMMAN *const man, unsigned int const addr, unsigned int const size);

// sheet.c
#define MAX_SHEETS 256 // 最大图层数
struct SHEET
{ // 图层信息
    struct SHTCTL *ctl;
    unsigned char *buf; // 内容地址
    int bxsize, bysize; // 图层大小
    int vx0, vy0;       // 图层左上角的屏幕坐标
    int col_inv;        // 透明色色号
    int height;         // 高度
    int flags;          // 其他设定
};
struct SHTCTL
{                                     // 图层管理
    unsigned char *vram, *map;        // map与vram同大 用来表示画面上的每个点分别是哪个图层的像素
    int xsize, ysize;                 // VRAM尺寸
    int top;                          // 最上面图层的高度
    struct SHEET *sheets[MAX_SHEETS]; // 按高度升序排序的各图层信息的地址
    struct SHEET sheets0[MAX_SHEETS]; // 存放各图层信息
};
#define SHEET_USE 1 // flag: 正在使用的SHEET
struct SHTCTL *shtctl_init(struct MEMMAN *const memman, unsigned char *const vram, int xsize, int ysize);
struct SHEET *sheet_alloc(struct SHTCTL *const ctl);
void sheet_setbuf(struct SHEET *const sht, unsigned char *const buf, int const xsize, int const ysize, int const col_inv);
void sheet_updown(struct SHEET *const sht, int height);
void sheet_refreshmap(struct SHTCTL *const ctl, int vx0, int vy0, int vx1, int vy1, const int h0);
void sheet_refreshsub(struct SHTCTL const *const ctl, int vx0, int vy0, int vx1, int vy1, const int h0, const int h1);
void sheet_refresh(struct SHEET const *const sht, const int bx0, const int by0, const int bx1, const int by1);
void sheet_slide(struct SHEET *const sht, int const vx0, int const vy0);
void sheet_free(struct SHEET *const sht);

#endif
