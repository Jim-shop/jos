#ifndef __bootpack_h_
#define __bootpack_h_

// asmhead 中先前存的信息
#define ADR_BOOTINFO 0x00000ff0
struct BOOTINFO
{                                      // 位于0x0ff0~0x0fff
    const unsigned char CYLS;          // 读入的柱面数
    unsigned char LEDS;                // 键盘LED状态
    const unsigned char VMODE;         // 颜色位数
    const unsigned char _RESERVE;      // 结构体对齐
    const unsigned short SCRNX, SCRNY; // 屏幕分辨率
    unsigned char *const VRAM;         // 图像缓冲区开始地址
};
extern struct BOOTINFO *const binfo;
#define ADR_DISKIMG 0x00100000 // 磁盘内容被asmhead放在了此处

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
void load_tr(const int tr);
void asm_inthandler20(void);
void asm_inthandler21(void);
void asm_inthandler27(void);
void asm_inthandler2c(void);
unsigned int asm_memtest_sub(unsigned int start, unsigned int end); //已用C语言实现，仅做备份
void farjmp(int eip, int cs);

// font 提供的字库
extern const char font[4096];

// GOlib 头文件
#include <stdio.h>
#include <string.h>

// bootpack.c
#define KEYCMD_LED 0xed // 键盘灯设置端口

// graphic.c
void init_palette(void);
void set_palette(int start, const int end, unsigned char *rgb);
void boxfill8(unsigned char *const vram, const int xsize, unsigned char const c, int const x0, int y0, int const x1, int const y1);
void init_screen8(unsigned char *const vram, int const x, int const y);
void putfont8(unsigned char *const vram, int const xsize, int const x, int const y, char const c, char const *const font);
void putfonts8_asc(unsigned char *const vram, const int xsize, int x, const int y, const char c, char const *s);
void init_mouse_cursor8(char *const mouse, const char bc);
void putblock8_8(unsigned char *const vram, const int vxsize, const int pxsize, const int pysize, const int px0, const int py0, char const *const buf, const int bxsize);
// 默认调色板颜色
#define black 0          // 000000 黑
#define red 1            // ff0000 亮红
#define green 2          // 00ff00 亮绿
#define yellow 3         // ffff00 亮黄
#define blue 4           // 0000ff 亮蓝
#define purple 5         // ff00ff 亮紫
#define lightblue 6      // 00ffff 浅亮蓝
#define white 7          // ffffff 白
#define gray 8           // c6c6c6 亮灰
#define darkred 9        // 840000 暗红
#define darkgreen 10     // 008400 暗绿
#define darkyellow 11    // 848400 暗黄
#define darkblue 12      // 000084 暗青
#define darkpurple 13    // 840084 暗紫
#define lightdarkblue 14 // 008484 浅暗蓝
#define darkgray 15      // 848484 暗灰

// dsctbl.c
#define ADR_IDT 0x0026f800   // 任选的，储存IDT表的位置
#define LIMIT_IDT 0x000007ff // IDT表的项数
#define ADR_GDT 0x00270000   // 任选的，储存GDT表的位置
#define LIMIT_GDT 0x0000ffff // GDT表的项数
#define ADR_BOTPAK 0x00280000
#define LIMIT_BOTPAK 0x0007ffff
#define AR_DATA32_RW 0x4092
#define AR_CODE32_ER 0x409a
#define AR_TSS32 0x0089     // TSS32的访问权限
#define AR_INTGATE32 0x008e // IDT属性，表示用于中断处理的有效设定
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
extern struct SEGMENT_DESCRIPTOR *gdt;
extern struct GATE_DESCRIPTOR *idt;
void init_gdtidt(void);
void set_segmdesc(struct SEGMENT_DESCRIPTOR *const sd, unsigned int limit, const int base, int ar);
void set_gatedesc(struct GATE_DESCRIPTOR *const gd, const int offset, const int selector, const int ar);

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

// fifo.c
struct FIFO32
{
    int *buf;
    // 队尾，队头，缓冲区总大小，缓冲区空闲数，标志位
    int end, start, size, free, flags;
    struct TASK *task; // 要唤醒的任务
};
void fifo32_init(struct FIFO32 *const fifo, const int size, int *const buf, struct TASK *const task);
int fifo32_put(struct FIFO32 *const fifo, int const data);
int fifo32_get(struct FIFO32 *const fifo);
int fifo32_status(struct FIFO32 const *const fifo);
#define FLAGS_OVERRUN 0x0001

// keyboard.c
extern struct FIFO32 *keyfifo;
void wait_KBC_sendready(void);
void init_keyboard(struct FIFO32 *const fifo, const int data0);
void inthandler21(int *esp);
#define PORT_KEYDAT 0x0060        // 键盘输入的数据（编码）
#define PORT_KEYSTA 0x0064        // 键盘控制电路的状态输出设备号
#define PORT_KEYCMD 0x0064        // 键盘控制电路的指令设置设备号
#define KEYSTA_SEND_NOTREADY 0x02 // KBC状态输出数据的掩码，得到倒数第二位的值
#define KEYCMD_WRITE_MODE 0x60    // 进入设定模式的指令
#define KBC_MODE 0x47             // 鼠标模式

// mouse.c
extern struct FIFO32 *mousefifo;
struct MOUSE_DEC
{
    /*
    除了初始化时产生的0xfa，鼠标发送的数据是3字节1组
    */
    // phase: 0等待0xfa, 1,2,3表示等待第几字节
    unsigned char buf[3], phase;
    int x, y, btn;
};
void enable_mouse(struct FIFO32 *const fifo, const int data0, struct MOUSE_DEC *const mdec);
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
inline unsigned int memtest_sub(unsigned int start, unsigned const int end);
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
#define SHEET_FLAGS_USING 1 // flag: 正在使用的SHEET
#define SHEET_FLAGS_FREE 0  // 未被使用
struct SHTCTL *shtctl_init(struct MEMMAN *const memman, unsigned char *const vram, int xsize, int ysize);
struct SHEET *sheet_alloc(struct SHTCTL *const ctl);
void sheet_setbuf(struct SHEET *const sht, unsigned char *const buf, int const xsize, int const ysize, int const col_inv);
void sheet_updown(struct SHEET *const sht, int height);
void sheet_refreshmap(struct SHTCTL *const ctl, int vx0, int vy0, int vx1, int vy1, const int h0);
void sheet_refreshsub(struct SHTCTL const *const ctl, int vx0, int vy0, int vx1, int vy1, const int h0, const int h1);
void sheet_refresh(struct SHEET const *const sht, const int bx0, const int by0, const int bx1, const int by1);
void sheet_slide(struct SHEET *const sht, int const vx0, int const vy0);
void sheet_free(struct SHEET *const sht);

// timer.c
#define MAX_TIMER 500
struct TIMER
{
    struct TIMER *next;          // 下一个触发的timer
    unsigned int timeout, flags; // timeout: 设定时刻（绝对时间）
    struct FIFO32 *fifo;         // 一旦达到timeout，就往FIFO内发送data的数据
    int data;
};
struct TIMERCTL
{
    unsigned int count; // 计时
    unsigned int next;  // 下一个timer的时间点
    struct TIMER *t0;   // 链表，按timeout升序排列
    struct TIMER timers0[MAX_TIMER];
};
extern struct TIMERCTL timerctl;
#define PIT_CTRL 0x0043     // PIT控制设备号
#define PIT_CNT0 0x0040     // PIT中断频率设置设备号
#define TIMER_FLAGS_FREE 0  // 状态：未配置
#define TIMER_FLAGS_ALLOC 1 // 状态：已配置
#define TIMER_FLAGS_USING 2 // 状态：运行中
void init_pit(void);
void inthandler20(int *esp);
struct TIMER *timer_alloc(void);
void timer_free(struct TIMER *const timer);
void timer_init(struct TIMER *const timer, struct FIFO32 *const fifo, unsigned int const data);
void timer_settime(struct TIMER *const timer, unsigned int const timeout);

// mtask.c
#define MAX_TASKS 1000    // 最大任务数
#define TASK_GDT0 3       // 定义从GDT几号开始分配给TSS
#define MAX_TASKS_LV 100  // 每个级别能运行的最大任务数
#define MAX_TASKLEVELS 10 //多少个级别
struct TSS32
{                                                            // TSS 任务代码段。CPU JMP到TSS上时就会执行任务切换。
    int backlink, esp0, ss0, esp1, ss1, esp2, ss2, cr3;      // 任务设置相关信息
    int eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi; // 保存32位寄存器状态
    int es, cs, ss, ds, fs, gs;                              // 保存16位寄存器的状态
    int ldtr, iomap;                                         // ldtr=0, iomap=0x40000000
};
struct TASK
{
    int sel, flags;      // sel存放GDT编号(已乘8)
    int level, priority; // 优先级、每片时长
    struct FIFO32 fifo;
    struct TSS32 tss;
};
struct TASKLEVEL
{
    int running; // 正在运行的任务数量
    int now;     // 用来记录当前正在运行的是哪个任务
    struct TASK *tasks[MAX_TASKS_LV];
};
struct TASKCTL
{
    int now_lv;     // 现在活动中的LEVEL
    char lv_change; // 下次任务切换时是否需要改变LEVEL
    struct TASKLEVEL level[MAX_TASKLEVELS];
    struct TASK tasks0[MAX_TASKS];
};
#define TASK_FLAGS_FREE 0        // 未分配
#define TASK_FLAGS_SLEEP 1       // 分配，暂未执行
#define TASK_FLAGS_RUNNING 2     // 活动进程
extern struct TIMER *task_timer; // 任务切换定时器
extern struct TASKCTL *taskctl;
struct TASK *task_now(void);
void task_add(struct TASK *const task);
void task_remove(struct TASK *const task);
void task_switchsub(void);
struct TASK *task_init(struct MEMMAN *const memman);
struct TASK *task_alloc(void);
void task_run(struct TASK *const task, int level, const int priority);
void task_switch(void);
void task_sleep(struct TASK *const task);
void task_idle(void);

// window.c
void make_window8(unsigned char *const buf, const int xsize, const int ysize, char const *const title, char const act);
void make_wtitle8(unsigned char *const buf, const int xsize, char const *const title, char const act);
void make_textbox8(struct SHEET *const sht, const int x0, const int y0, const int sx, const int sy, const int c);
void putfont8_sht(struct SHEET *const sht, const int x, const int y, const int c, const int b, const char ch);
void putfonts8_asc_sht(struct SHEET *const sht, const int x, const int y, const int c, const int b, char const *const s, const int l);

// file.c
struct FILEINFO
{														 // FAT12文件表单项内容（最多224个）
	unsigned char name[8];		 // 文件名。第一字节0xe5代表删除了,0x00代表不包含文件信息。
	unsigned char ext[3];			 // 扩展名
	unsigned char type;				 // 属性信息：0x00/0x20一般文件0x01只读0x02隐藏0x04系统0x08非文件信息(磁盘名称等)0x10目录（可以叠加）
	unsigned char reserve[10]; // 保留无用的10字节
	unsigned short time, date; // 时间，日期
	unsigned short clustno;		 // 簇号(小端序，地址=簇号*512+0x3e00)
	unsigned int size;				 // 大小
};
void file_readfat(unsigned short *const fat, unsigned char const *const img);
void file_loadfile(unsigned int clustno, unsigned int size, char *buf, unsigned short const *const fat, unsigned char const *img);

// console.c
void console_task(struct SHEET *const sheet, unsigned int const memtotal);
int cons_newline(int cursor_y, struct SHEET *const sheet);

#endif
