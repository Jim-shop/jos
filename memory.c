/*
内存管理表维护。
*/

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

#include "bootpack.h"

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
    volatile unsigned int eflg = io_load_eflags();
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
            return a;
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
        for (j = man->frees; j > i; j--)
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

unsigned int memman_alloc_4k(struct MEMMAN *const man, unsigned int const size)
{
    /*
    以4KB为单位申请内存（将size向上舍入）。
    */
    return memman_alloc(man, (size + 0xfff) & 0xfffff000);
}

int memman_free_4k(struct MEMMAN *const man, unsigned int const addr, unsigned int const size)
{
    /*
    以4KB为单位释放内存。返回值同memman_free()：
    0表示正常，-1表示失败。
    */
    return memman_free(man, addr, (size + 0xfff) & 0xfffff000);
}
