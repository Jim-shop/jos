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
void io_out8(int port, int data);
int io_load_eflags(void);
void io_store_eflags(const int eflags);
void load_gdtr(const int limit, const int addr);
void load_idtr(const int limit, const int addr);
void asm_inthandler21(void);
void asm_inthandler27(void);
void asm_inthandler2c(void);

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
{ // 默认调色板颜色
    black = 0,
    red = 1,
    green = 2,
    yellow = 3,
    blue = 4,
    purple = 5,
    lightblue = 6,
    white = 7,
    gray = 8,
    darkred = 9,
    darkgreen = 10,
    darkyellow = 11,
    darkblue = 12,
    darkpurple = 13,
    lightdarkblue = 14,
    darkgray = 15
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
void inthandler21(int *esp);
void inthandler27(int *esp);
void inthandler2c(int *esp);
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

#endif
