/*
读取文件相关。
*/

#include "bootpack.h"

void file_readfat(unsigned short *const fat, unsigned char const *const img)
{
    /*
    读取并解码FAT12表。
    fat指针：保存解码结果
    img指针：软盘中FAT表地址。
    */
    /*
    FAT12: 低端 中端 高端（含12bit记录2条）
                 ab   cd   ef
    解码结果：低端  高端
                     dab  efc
    */
    short i, j = 0;
    for (i = 0; i < 2880; i += 2) // 磁盘中共有2880个扇区
    {
        fat[i + 0] = (img[j + 0] | img[j + 1] << 8) & 0xfff;
        fat[i + 1] = (img[j + 1] >> 4 | img[j + 2] << 4) & 0xfff;
        j += 3;
    }
    return;
}

void file_loadfile(unsigned int clustno, unsigned int size, char *buf, unsigned short const *const fat, unsigned char const *img)
{
    /*
    按照fat表读取文件内容。
    clustno: 起始簇号
    size: 文件大小
    buf: 读入缓存区域
    fat: fat表
    img: 文件储存区(0x003e00)处位置。
    */
    int i;
    for (;;)
    {
        if (size <= 512)
        {
            for (i = 0; i < size; i++)
                buf[i] = img[clustno * 512 + i];
            break;
        }
        for (i = 0; i < 512; i++)
            buf[i] = img[clustno * 512 + i];
        size -= 512;
        buf += 512;
        clustno = fat[clustno];
    }
    return;
}

struct FILEINFO *file_search(char const *const name, struct FILEINFO *const finfo, const int max)
{
    /*
    根据文件名查找文件。返回匹配的文件finfo指针，无匹配时返回NULL。
    finfo: 文件表。
    max: 文件表中文件总项数。
    */

    // 临时变量
    int i, j = 0;
    char s[12];
    for (i = 0; i < 11; i++)
        s[i] = ' ';
    // 将输入文件名格式规范一下
    for (i = 0; name[i] != 0; i++)
    {
        if (j >= 11)     // 超过正常的文件名长度
            return NULL; // 没有找到
        if (name[i] == '.' && j <= 8)
            j = 8;
        else
        {
            s[j] = name[i];
            if ('a' <= s[j] && s[j] <= 'z')
                s[j] -= 0x20; // 将小写字母转换为大写字母
            j++;
        }
    }
    // 查找文件
    for (i = 0; i < max;)
    {
        if (finfo[i].name[0] == 0x00) // 没有更多文件了
            break;
        if ((finfo[i].type & 0x18) == 0) // 不是目录或非文件信息
        {
            for (j = 0; j < 11; j++)
                if (finfo[i].name[j] != s[j])
                    goto next; // 不匹配
            return finfo + i;  // 找到了
        }
    next:
        i++;
    }
    return NULL;
}
