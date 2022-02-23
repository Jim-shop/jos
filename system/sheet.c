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
    ctl->map = (unsigned char *)memman_alloc_4k(memman, xsize * ysize);
    if (ctl->map == NULL)
    {
        memman_free_4k(memman, (unsigned int)ctl, sizeof(struct SHTCTL));
        return ctl;
    }
    ctl->vram = vram;
    ctl->xsize = xsize;
    ctl->ysize = ysize;
    ctl->top = -1; // 一个SHEET都无
    int i;
    for (i = 0; i < MAX_SHEETS; i++)
    {
        ctl->sheets0[i].flags = SHEET_FLAGS_FREE;
        ctl->sheets0[i].ctl = ctl;
    }
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
        if (ctl->sheets0[i].flags == SHEET_FLAGS_FREE)
        {
            sht = &ctl->sheets0[i];
            sht->flags = SHEET_FLAGS_USING;
            sht->height = -1; // 隐藏
            sht->task = 0;    // 不使用自动关闭功能
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

void sheet_updown(struct SHEET *const sht, int height)
{
    /*
    设定图层高度，顺便对指针表sheets[]做排序
    */

    // 记录设置之前的高度信息
    struct SHTCTL *const ctl = sht->ctl;
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
            sheet_refreshmap(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, height + 1);
            sheet_refreshsub(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, height + 1, old);
        }
        else // -1 转隐藏模式
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
            sheet_refreshmap(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, 0);
            sheet_refreshsub(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, 0, old - 1);
        }
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
        sheet_refreshmap(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, height);
        sheet_refreshsub(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, height, height);
    }

    return;
}

void sheet_refresh(struct SHEET const *const sht, const int bx0, const int by0, const int bx1, const int by1)
{
    /*
    刷新指定图层的指定区域。
    指定的两个坐标（超尾）是相对于图层原点的坐标。
    */
    if (sht->height >= 0) // 如果图层可见，就重画这一层的对应区域（此函数为了性能，没有考虑透明的情况）
        sheet_refreshsub(sht->ctl, sht->vx0 + bx0, sht->vy0 + by0, sht->vx0 + bx1, sht->vy0 + by1, sht->height, sht->height);
    return;
}

void sheet_refreshsub(struct SHTCTL const *const ctl, int vx0, int vy0, int vx1, int vy1, const int h0, const int h1)
{
    /*
    从下到上在指定区域(超尾)绘制:高度在h0（含）至h1（含）之间的所有可见图层
        v打头变量：实际屏幕坐标
        b打头变量：相对于图层原点的坐标
    */
    int h, bx, by, vx, vy, bx0, by0, bx1, by1;
    unsigned char *vram = ctl->vram;
    unsigned char *map = ctl->map;
    struct SHEET *sht;
    unsigned char *buf;
    unsigned char sid;

    char *pmap, *pvram, *pbuf;

    if (vx0 < 0)
        vx0 = 0;
    if (vy0 < 0)
        vy0 = 0;
    if (vx1 > ctl->xsize)
        vx1 = ctl->xsize;
    if (vy1 > ctl->ysize)
        vy1 = ctl->ysize;

    for (h = h0; h <= h1; h++)
    { // 从h0图层开始绘制
        sht = ctl->sheets[h];
        buf = sht->buf;
        sid = sht - ctl->sheets0;

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

        if ((sht->vx0 & 3) == 0) // 先前4对齐了（证明很有可能可以进行快速处理）
        {
            int i = (bx0 + 3) / 4; // 小数进位
            int j = bx1 / 4 - i;   // 小数舍去
            int sid4 = sid | sid << 8 | sid << 16 | sid << 24;
            int *pmap4, *pvram4, *pbuf4;
            int bx2;
            for (by = by0, vy = sht->vy0 + by0; by < by1; by++, vy++)
            {
                pmap = &map[vy * ctl->xsize];
                pvram = &vram[vy * ctl->xsize];
                pbuf = &buf[by * sht->bxsize];
                // 前面被4除多余的部分逐个字节写入
                for (bx = bx0, vx = sht->vx0 + bx0; bx < bx1 && (bx & 3) != 0; bx++, vx++)
                    if (pmap[vx] == sid)
                        pvram[vx] = pbuf[bx];
                // 4倍数部分
                pmap4 = (int *)&map[vy * ctl->xsize + vx];
                pvram4 = (int *)&vram[vy * ctl->xsize + vx];
                pbuf4 = (int *)&buf[by * sht->bxsize + bx];
                for (i = 0; i < j; i++)
                {
                    if (pmap4[i] == sid4)
                        pvram4[i] = pbuf4[i];
                    else
                    {
                        bx2 = bx + i * 4;
                        vx = sht->vx0 + bx2;
                        if (pmap[vx + 0] == sid)
                            pvram[vx + 0] = pbuf[bx2 + 0];
                        if (pmap[vx + 1] == sid)
                            pvram[vx + 1] = pbuf[bx2 + 1];
                        if (pmap[vx + 2] == sid)
                            pvram[vx + 2] = pbuf[bx2 + 2];
                        if (pmap[vx + 3] == sid)
                            pvram[vx + 3] = pbuf[bx2 + 3];
                    }
                }
                // 后面被4除多余的部分逐个字节写入
                for (bx += j * 4, vx = sht->vx0 + bx; bx < bx1; bx++, vx++)
                    if (pmap[vx] == sid)
                        pvram[vx] = pbuf[bx];
            }
        }
        else //  1字节
        {
            for (by = by0, vy = sht->vy0 + by0; by < by1; by++, vy++)
            {
                pmap = &map[vy * ctl->xsize];
                pvram = &vram[vy * ctl->xsize];
                pbuf = &buf[by * sht->bxsize];
                for (bx = bx0, vx = sht->vx0 + bx0; bx < bx1; bx++, vx++)
                    if (pmap[vx] == sid)
                        pvram[vx] = pbuf[bx];
            }
        }
    }
    return;
}

void sheet_refreshmap(struct SHTCTL *const ctl, int vx0, int vy0, int vx1, int vy1, const int h0)
{
    /*
    从h0高度及以上刷新map。
    map与vram同大，用来记录画面中的点是哪个图层的像素
    */
    int h, bx, by, vx, vy, bx0, by0, bx1, by1;
    unsigned char *map = ctl->map;
    struct SHEET *sht;
    unsigned char *buf;
    unsigned char sid;
    int sid4, *p4;
    char *pbuf, *pmap;

    if (vx0 < 0)
        vx0 = 0;
    if (vy0 < 0)
        vy0 = 0;
    if (vx1 > ctl->xsize)
        vx1 = ctl->xsize;
    if (vy1 > ctl->ysize)
        vy1 = ctl->ysize; // 绘制目标位置

    for (h = h0; h <= ctl->top; h++)
    {
        sht = ctl->sheets[h];
        sid = sht - ctl->sheets0; // 图层在sheets中的位置
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
            by1 = sht->bysize; // 相对位置

        if (sht->col_inv == -1) // 没有透明色
        {
            if ((sht->vx0 & 3) == 0 && (bx0 & 3) == 0 && (bx1 & 3) == 0) // 是4倍数
            {                                                            // 启用4字节优化
                bx1 = (bx1 - bx0) / 4;
                sid4 = sid | sid << 8 | sid << 16 | sid << 24;
                for (by = by0, vy = sht->vy0 + by0; by < by1; by++, vy++)
                {
                    p4 = (int *)&map[vy * ctl->xsize + sht->vx0 + bx0];
                    for (bx = 0; bx < bx1; bx++)
                        p4[bx] = sid4;
                }
            }
            else // 无4字节优化、有透明优化
            {
                for (by = by0, vy = sht->vy0 + by0; by < by1; by++, vy++)
                {
                    pmap = &map[vy * ctl->xsize];
                    for (bx = bx0, vx = sht->vx0 + bx0; bx < bx1; bx++, vx++)
                        pmap[vx] = sid;
                }
            }
        }
        else // 有透明色，无优化
        {
            for (by = by0, vy = sht->vy0 + by0; by < by1; by++, vy++)
            {

                pbuf = &buf[by * sht->bxsize];
                pmap = &map[vy * ctl->xsize];
                for (bx = bx0, vx = sht->vx0 + bx0; bx < bx1; bx++, vx++)
                    if (pbuf[bx] != sht->col_inv)
                        pmap[vx] = sid;
            }
        }
    }
    return;
}

void sheet_slide(struct SHEET *const sht, int const vx0, int const vy0)
{
    /*
    上下左右移动图层。不改变图层高度。给定图层左上坐标
    */
    int old_vx0 = sht->vx0, old_vy0 = sht->vy0;
    sht->vx0 = vx0;
    sht->vy0 = vy0;
    if (sht->height >= 0) // 如果是可见图层
    {
        sheet_refreshmap(sht->ctl, old_vx0, old_vy0, old_vx0 + sht->bxsize, old_vy0 + sht->bysize, 0);                  // 原有的区域的某一层变了，要确定整个画面的像素归属
        sheet_refreshmap(sht->ctl, vx0, vy0, vx0 + sht->bxsize, vy0 + sht->bysize, sht->height);                        // 图层移到的地方，图层高度以下的像素归属不需要重新计算
        sheet_refreshsub(sht->ctl, old_vx0, old_vy0, old_vx0 + sht->bxsize, old_vy0 + sht->bysize, 0, sht->height - 1); // 重画原有图层高度以下的层所支配的像素
        sheet_refreshsub(sht->ctl, vx0, vy0, vx0 + sht->bxsize, vy0 + sht->bysize, sht->height, sht->height);           // 不会有图层高度以下的像素新露出来，因此不用管图层高度以下的像素
    }
    return;
}

void sheet_free(struct SHEET *const sht)
{
    /*
    释放已使用的图层
    */
    if (sht->height >= 0)
        sheet_updown(sht, -1);
    sht->flags = SHEET_FLAGS_FREE;
    return;
}
