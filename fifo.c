/*
队列
*/

#include "bootpack.h"

void fifo8_init(struct FIFO8 *const fifo, const int size, unsigned char *const buf)
{
    /*
    初始化FIFO缓冲区
    */
    fifo->size = size;
    fifo->buf = buf;
    fifo->free = size;
    fifo->flags = 0;
    fifo->end = 0;
    fifo->start = 0;
    return;
}

int fifo8_put(struct FIFO8 *const fifo, unsigned char const data)
{
    /*
    向FIFO传送数据并保存。返回值0代表执行成功，-1表示溢出
    */
    if (fifo->free == 0)
    {
        fifo->flags |= FLAGS_OVERRUN;
        return -1;
    }
    fifo->buf[fifo->end] = data;
    fifo->end++;
    if (fifo->end == fifo->size)
        fifo->end = 0;
    fifo->free--;
    return 0;
}

int fifo8_get(struct FIFO8 *const fifo)
{
    /* 
    从FIFO取得一个数据。返回-1表示队列为空，否则返回取到的值
     */
    int data;
    if (fifo->free == fifo->size)
        return -1;
    data = fifo->buf[fifo->start];
    fifo->start++;
    if (fifo->start == fifo->size)
        fifo->start = 0;
    fifo->free++;
    return data;
}

int fifo8_status(struct FIFO8 *const fifo)
{
    /*
    返回缓冲区内待处理的数据量
    */
    return fifo->size - fifo->free;
}
