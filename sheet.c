/*
图层
*/

#include "bootpack.h"

struct SHTCTL *shtctl_init(struct MEMMAN *const memman, unsigned char *const vram, int xsize, int ysize)
{
    /*
    初始化图层控制表。申请不到内存时返回空指针，
    否则返回一个内容初始化为空的SHTCTL的指针。
    */
    struct SHTCTL *ctl = (struct SHTCTL *)memman_alloc_4k(memman, sizeof(struct SHTCTL));
    if (ctl == 0)
        return ctl;
    ctl->vram = vram;
    ctl->xsize = xsize;
    ctl->ysize = ysize;
    ctl->top = -1; // 一个SHEET都无
    int i;
    for (i = 0; i < MAX_SHEETS; i++)
        ctl->sheets0[i].flags = 0;
    return ctl;
}

struct SHEET *sheet_alloc(struct SHTCTL *const ctl)
{
    /*
    从SHTCTL->sheets0中获取一个未被使用的图层，
    返回sheet指针。若都被使用，返回空指针。
    */
    struct SHEET *sht;
    int i;
    for (i = 0; i < MAX_SHEETS; i++)
        if (ctl->sheets0[i].flags == 0)
        {
            sht = &ctl->sheets0[i];
            sht->flags = SHEET_USE;
            sht->height = -1; // 隐藏
            return sht;
        }
    return NULL;
}

void sheet_setbuf(struct SHEET *const sht, unsigned char *const buf, int const xsize, int const ysize, int const col_inv)
{
    /*
    设置图层基础信息
    */
    sht->buf = buf;
    sht->bxsize = xsize;
    sht->bysize = ysize;
    sht->col_inv = col_inv;
    return;
}

void sheet_updown(struct SHTCTL *const ctl, struct SHEET *const sht, int height)
{
    /*
    设定图层高度，顺便对指针表sheets[]做排序
    */

    // 设置之前的高度信息
    int old = sht->height;

    // 修正高度离谱的数据
    if (height > ctl->top + 1)
        height = ctl->top + 1;
    if (height < -1)
        height = -1;

    // 设定高度
    sht->height = height;

    // 排序sheets[]
    int h;
    if (old > height) //比以前低
    {
        if (height >= 0)
        {
            for (h = old; h > height; h--)
            {
                ctl->sheets[h] = ctl->sheets[h - 1];
                ctl->sheets[h]->height = h;
            }
            ctl->sheets[height] = sht;
        }
        else // -1 隐藏模式
        {
            if (ctl->top > old) // 不在最上层
            {
                for (h = old; h < ctl->top; h++)
                {
                    ctl->sheets[h] = ctl->sheets[h + 1];
                    ctl->sheets[h]->height = h;
                }
            }
            ctl->top--;
        }
        sheet_refreshsub(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize);
    }
    else if (old < height) // 比以前高
    {
        if (old >= 0)
        {
            for (h = old; h < height; h++)
            {
                ctl->sheets[h] = ctl->sheets[h + 1];
                ctl->sheets[h]->height = h;
            }
            ctl->sheets[height] = sht;
        }
        else // 隐藏模式转显示
        {
            for (h = ctl->top; h >= height; h--)
            {
                ctl->sheets[h + 1] = ctl->sheets[h];
                ctl->sheets[h + 1]->height = h + 1;
            }
            ctl->sheets[height] = sht;
            ctl->top++;
        }
        sheet_refreshsub(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize);
    }

    return;
}

void sheet_refresh(struct SHTCTL const *const ctl, struct SHEET const *const sht, const int bx0, const int by0, const int bx1, const int by1)
{
    /*
    刷新指定图层的指定区域。
    指定的两个坐标是相对于图层原点的坐标。
    */
    if (sht->height >= 0) // 如果图层可见
        sheet_refreshsub(ctl, sht->vx0 + bx0, sht->vy0 + by0, sht->vx0 + bx1, sht->vy0 + by1);
    return;
}

void sheet_refreshsub(struct SHTCTL const *const ctl, const int vx0, const int vy0, const int vx1, const int vy1)
{
    /*
    从下到上在指定区域绘制所有可见图层
        v打头变量：实际屏幕坐标
        b打头变量：相对于图层原点的坐标
    */
    int h, bx, by, vx, vy, bx0, by0, bx1, by1;
    unsigned char *vram = ctl->vram;
    struct SHEET *sht;
    unsigned char *buf;
    unsigned char c ;
    for (h = 0; h <= ctl->top; h++)
    { // 从最下图层开始绘制
        sht = ctl->sheets[h];
        buf = sht->buf;

        bx0 = vx0 - sht->vx0;
        by0 = vy0 - sht->vy0;
        bx1 = vx1 - sht->vx0;
        by1 = vy1 - sht->vy0;
        if (bx0 < 0)
            bx0 = 0;
        if (by0 < 0)
            by0 = 0;
        if (bx1 > sht->bxsize)
            bx1 = sht->bxsize;
        if (by1 > sht->bysize)
            by1 = sht->bysize;

        for (by = by0; by < by1; by++)
        {
            vy = sht->vy0 + by;
            for (bx = bx0; bx < bx1; bx++)
            {
                vx = sht->vx0 + bx;
                c = buf[by * sht->bxsize + bx];
                if (c != sht->col_inv)
                    vram[vy * ctl->xsize + vx] = c;
            }
        }
    }
    return;
}

void sheet_slide(struct SHTCTL const *const ctl, struct SHEET *const sht, int const vx0, int const vy0)
{
    /*
    上下左右移动图层。不改变图层高度。给定图层左上坐标
    */
    int old_vx0 = sht->vx0, old_vy0 = sht->vy0;
    sht->vx0 = vx0;
    sht->vy0 = vy0;
    if (sht->height >= 0) // 如果是可见图层
    {
        sheet_refreshsub(ctl, old_vx0, old_vy0, old_vx0 + sht->bxsize, old_vy0 + sht->bysize);
        sheet_refreshsub(ctl, vx0, vy0, vx0 + sht->bxsize, vy0 + sht->bysize);
    }
    return;
}

void sheet_free(struct SHTCTL *const ctl, struct SHEET *const sht)
{
    /*
    释放已使用的图层
    */
    if (sht->height >= 0)
        sheet_updown(ctl, sht, -1);
    sht->flags = 0;
    return;
}
