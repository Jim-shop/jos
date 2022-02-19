/*
提供控制台任务。
*/

#include "bootpack.h"

int cons_newline(int cursor_y, struct SHEET *const sheet)
{
	/*
	给控制台换行。行满时滚动。返回换行之后cursor_y位置。
	使用前记得把光标擦除。
	*/
	int x, y;
	if (cursor_y < 28 + 112) // 没填满所有行
		cursor_y += 16;				 // 换行
	else										 // 填满了所有行
	{
		for (y = 28; y < 28 + 112; y++)
			for (x = 8; x < 8 + 240; x++)
				sheet->buf[x + y * sheet->bxsize] = sheet->buf[x + (y + 16) * sheet->bxsize]; // 滚动
		for (y = 28 + 112; y < 28 + 128; y++)
			for (x = 8; x < 8 + 240; x++)
				sheet->buf[x + y * sheet->bxsize] = black; // 清空
		sheet_refresh(sheet, 8, 28, 8 + 240, 28 + 128);
	}
	return cursor_y;
}

void console_task(struct SHEET *const sheet, unsigned int const memtotal)
{
	/*
	控制台进程。
	*/

	// 自身task
	struct TASK *task = task_now();

	// 事件队列
	int fifobuf[128];
	fifo32_init(&task->fifo, 128, fifobuf, task);

	// 光标
	int cursor_x = 16, cursor_y = 28, cursor_c = -1;
	struct TIMER *timer = timer_alloc();
	timer_init(timer, &task->fifo, 1);
	timer_settime(timer, 50);

	// 暂存指令
	char cmdline[30];

	// 临时变量
	int x, y;
	char s[30];
	char *p;

	// mem 指令用
	struct MEMMAN *const memman = (struct MEMMAN *const)MEMMAN_ADDR;

	// dir 指令用
	struct FILEINFO *const finfo = (struct FILEINFO *const)(ADR_DISKIMG + 0x002600);

	// fat表 0柱面0磁头2~9扇区第一份10~18第二份（完全相同）
	unsigned short *const fat = (unsigned short *const)memman_alloc_4k(memman, 2 * 2880); // 共2880扇区
	file_readfat(fat, (unsigned char *)(ADR_DISKIMG + 0x000200));

	// 提示符
	putfont8_sht(sheet, 8, 28, white, black, '>');

	int i;
	for (;;)
	{
		io_cli();
		if (fifo32_status(&task->fifo) == 0)
		{
			task_sleep(task);
			io_sti();
		}
		else
		{
			i = fifo32_get(&task->fifo);
			io_sti();
			switch (i)
			{
			case 0 ... 1: // 光标闪烁
				if (i != 0)
				{
					timer_init(timer, &task->fifo, 0);
					if (cursor_c >= 0)
						cursor_c = white;
				}
				else
				{
					timer_init(timer, &task->fifo, 1);
					if (cursor_c >= 0)
						cursor_c = black;
				}
				timer_settime(timer, 50);
				break;

			case 2: // 光标ON
				cursor_c = white;
				break;

			case 3: // 光标OFF
				boxfill8(sheet->buf, sheet->bxsize, black, cursor_x, cursor_y, cursor_x + 7, cursor_y + 15);
				cursor_c = -1;
				break;

			case 256 ... 511: // 键盘数据（从任务A得到）
				switch (i)
				{
				case '\b' + 256: // 退格信号
					if (cursor_x > 16)
					{
						putfont8_sht(sheet, cursor_x, cursor_y, white, black, ' ');
						cursor_x -= 8;
					}
					break;

				case '\n' + 256:																							// 回车信号
					putfont8_sht(sheet, cursor_x, cursor_y, white, black, ' '); // 擦除光标
					cmdline[cursor_x / 8 - 2] = 0;															// 字串结束标志
					cursor_y = cons_newline(cursor_y, sheet);
					if (strcmp(cmdline, "mem") == 0) // mem指令
					{																 // 指令功能：查询总计内存和剩余内存
						sprintf(s, "total   %dMB", memtotal / (1024 * 1024));
						putfonts8_asc_sht(sheet, 8, cursor_y, white, black, s, 30);
						cursor_y = cons_newline(cursor_y, sheet);
						sprintf(s, "free %dKB", memman_total(memman) / 1024);
						putfonts8_asc_sht(sheet, 8, cursor_y, white, black, s, 30);
						cursor_y = cons_newline(cursor_y, sheet);
						cursor_y = cons_newline(cursor_y, sheet);
					}
					else if (strcmp(cmdline, "cls") == 0) // cls 命令
					{																			// 功能：清屏
						for (y = 28; y < 28 + 128; y++)
							for (x = 8; x < 8 + 240; x++)
								sheet->buf[x + y * sheet->bxsize] = black;
						sheet_refresh(sheet, 8, 28, 8 + 240, 28 + 128);
						cursor_y = 28;
					}
					else if (strcmp(cmdline, "dir") == 0) // dir 命令
					{																			// 显示文件信息
						for (x = 0; x < 224; x++)						// 文件表中最多224项文件
						{
							if (finfo[x].name[0] == 0x00) // 没有文件信息了
								break;
							if (finfo[x].name[0] != 0xe5) // 不是已删除的文件
							{
								if ((finfo[x].type & 0x18) == 0) // 不是目录或非文件信息
								{
									sprintf(s, "filename.ext   %7d", finfo[x].size);
									for (y = 0; y < 8; y++)
										s[y] = finfo[x].name[y]; // 文件名
									s[9] = finfo[x].ext[0];
									s[10] = finfo[x].ext[1];
									s[11] = finfo[x].ext[2]; // 扩展名
									putfonts8_asc_sht(sheet, 8, cursor_y, white, black, s, 30);
									cursor_y = cons_newline(cursor_y, sheet);
								}
							}
						}
						cursor_y = cons_newline(cursor_y, sheet);
					}
					else if (strncmp(cmdline, "type ", 5) == 0) // type 命令
					{
						// 得到文件名
						for (y = 0; y < 11; y++)
							s[y] = ' ';
						y = 0;
						for (x = 5; y < 11 && cmdline[x] != 0; x++)
							if (cmdline[x] == '.' && y <= 8)
								y = 8;
							else
							{
								s[y] = cmdline[x];
								if ('a' <= s[y] && s[y] <= 'z')
									s[y] -= 0x20; // 小写转大写
								y++;
							}
						// 寻找文件
						for (x = 0; x < 224;)
						{
							if (finfo[x].name[0] == 0x00) // 没有更多文件了
								break;
							if ((finfo[x].type & 0x18) == 0) // 不是目录或非文件信息
							{
								for (y = 0; y < 11; y++)
									if (finfo[x].name[y] != s[y])
										goto type_next_file; // 不匹配
								break;									 // 找到了
							}
						type_next_file:
							x++;
						}
						if (x < 224 && finfo[x].name[0] != 0x00) // 找到了
						{
							y = finfo[x].size;
							p = (char *)memman_alloc_4k(memman, finfo[x].size);
							file_loadfile(finfo[x].clustno, finfo[x].size, p, fat, (unsigned char *)(ADR_DISKIMG + 0x003e00));
							cursor_x = 8;
							for (y = 0; y < finfo[x].size; y++)
							{
								if (p[y] == '\t') // 制表符
									do
									{
										putfont8_sht(sheet, cursor_x, cursor_y, white, black, ' ');
										cursor_x += 8;
										if (cursor_x == 8 + 240)
										{
											cursor_x = 8;
											cursor_y = cons_newline(cursor_y, sheet);
										}
									} while ((cursor_x - 8) & 0x1f); // 输出空格，直到cursor_x被32整除（即四个空格对齐）
								else if (p[y] == '\n')						 // 换行
								{
									cursor_x = 8;
									cursor_y = cons_newline(cursor_y, sheet);
								}
								else if (p[y] == '\r') // 回车
								{
									// 忽略
								}
								else // 其他字符
								{
									putfont8_sht(sheet, cursor_x, cursor_y, white, black, p[y]);
									cursor_x += 8;
									if (cursor_x == 8 + 240) // 行满
									{
										cursor_x = 8;
										cursor_y = cons_newline(cursor_y, sheet);
									}
								}
							}
							memman_free_4k(memman, (unsigned int)p, finfo[x].size);
						}
						else // 没找到
						{
							putfonts8_asc_sht(sheet, 8, cursor_y, white, black, "File not found.", 15);
							cursor_y = cons_newline(cursor_y, sheet);
						}
						cursor_y = cons_newline(cursor_y, sheet);
					}
					else if (cmdline[0] != 0) // 不是命令也不是空行
					{
						putfonts8_asc_sht(sheet, 8, cursor_y, white, black, "Wrong instruction.", 18);
						cursor_y = cons_newline(cursor_y, sheet);
						cursor_y = cons_newline(cursor_y, sheet);
					}
					putfont8_sht(sheet, 8, cursor_y, white, black, '>'); // 显示提示符
					cursor_x = 16;
					break;

				default: // 可显示字符
					if (cursor_x < 240)
					{
						cmdline[cursor_x / 8 - 2] = i - 256; // 读入
						putfont8_sht(sheet, cursor_x, cursor_y, white, black, i - 256);
						cursor_x += 8;
					}
					break;
				}
				break;
			}
			// 光标
			if (cursor_c >= 0)
				boxfill8(sheet->buf, sheet->bxsize, cursor_c, cursor_x, cursor_y, cursor_x + 7, cursor_y + 15);
			sheet_refresh(sheet, cursor_x, cursor_y, cursor_x + 8, cursor_y + 16);
		}
	}
}
