/*
处理全局段号记录表GDT和中断记录表IDT
*/

#include "bootpack.h"

/*
GDT 8字节记录内容：
    段基址base：（段的地址）
为了与16位机器兼容，分为三段储存base_low[WORD], base_mid[BYTE], base_high[BYTE]
    段上限limit：（段有多少字节-1）
占20位，limit_low 16位+ limit_high 的低4位（高4位用来存段属性）。
由于20位只能表示1MB内存，于是在段属性中设置了一个标志位Gbit，
打开这个标志位时把limit的单位解释从字节改为页（1页=4KB）。
    段属性access_right：（访问权属性）
占12位。低8位在access_right中，高4位(扩展访问权)位于limit_high中的高四位。
可以把ar看成这样的16位：x0xx（16进制）内存上也确实是这样分布的。
高4位：GD00 （G=Gbit；D为段模式，1指32位模式，0指16位模式，但不能调用BIOS）
低8位：可以设置系统模式(0x9a)/应用模式(0xfa)，读写、执行权限等
*/

// 这俩地址是任选的无特殊用途的地址：
struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *)ADR_GDT;
struct GATE_DESCRIPTOR *idt = (struct GATE_DESCRIPTOR *)ADR_IDT;

void init_gdtidt(void)
{
    /*
    初始化全局段号记录表GDT和中断记录表IDT
    */

    int i;

    // GDT

    for (i = 0; i <= LIMIT_GDT / 8; i++)
        set_segmdesc(gdt + i, 0, 0, 0);
    // 段号为1的段：上限4GB，地址0，表示CPU所能管理的全部内存本身
    set_segmdesc(gdt + 1, 0xffffffff, 0x00000000, AR_DATA32_RW);
    // 段号为2的段：大小512KB，地址0x280000，为bootpack.hrb准备
    set_segmdesc(gdt + 2, LIMIT_BOTPAK, ADR_BOTPAK, AR_CODE32_ER);
    load_gdtr(LIMIT_GDT, ADR_GDT);

    // IDT

    for (i = 0; i <= LIMIT_IDT / 8; i++)
        set_gatedesc(idt + i, 0, 0, 0);
    // 中断号20：时钟
    set_gatedesc(idt + 0x20, (int)asm_inthandler20, 2 << 3, AR_INTGATE32);
    // 中断号21：PS/2键盘
    set_gatedesc(idt + 0x21, (int)asm_inthandler21, 2 << 3, AR_INTGATE32);
    // 中断号2c：PS/2鼠标
    set_gatedesc(idt + 0x2c, (int)asm_inthandler2c, 2 << 3, AR_INTGATE32);
    // 中断号27：
    set_gatedesc(idt + 0x27, (int)asm_inthandler27, 2 << 3, AR_INTGATE32);
    load_idtr(LIMIT_IDT, ADR_IDT);

    return;
}

void set_segmdesc(struct SEGMENT_DESCRIPTOR *const sd, unsigned int limit, const int base, int ar)
{
    /*
    设置GDT表中8字节项
        limit：上限，指段的字节数-1
        base：基址
        ar：访问权限
    */
    if (limit > 0xfffff)
    {
        ar |= 0x8000; // G_bit := 1
        limit >>= 3;  // limit /= 0x1000;
    }
    sd->limit_low = limit & 0xffff;
    sd->base_low = base & 0xffff;
    sd->base_mid = (base >> 16) & 0xff;
    sd->access_right = ar & 0xff;
    sd->limit_high = ((limit >> 16) & 0x0f) | ((ar >> 8) & 0xf0);
    sd->base_high = (base >> 24) & 0xff;
    return;
}

void set_gatedesc(struct GATE_DESCRIPTOR *const gd, const int offset, const int selector, const int ar)
{
    /*
    设置IDT表中8字节项
        selector: 段选择器，低三位有其他意义，需要将段号左移3位
        offset: 相对段选择器的偏移量
        ar: IDT属性
    */
    gd->offset_low = offset & 0xffff;
    gd->selector = selector;
    gd->dw_count = (ar >> 8) & 0xff;
    gd->access_right = ar & 0xff;
    gd->offset_high = (offset >> 16) & 0xffff;
    return;
}
