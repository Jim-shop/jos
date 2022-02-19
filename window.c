/*
绘制窗口相关。
*/

#include "bootpack.h"

void putfonts8_asc_sht(struct SHEET *const sht, const int x, const int y, const int c, const int b, char const *const s, const int l)
{
	/*
	在一个图层的某个位置打印字符串。
	x,y: 坐标   c: 颜色   b: 背景颜色   s: 字串   l: 字串长度
	*/
	boxfill8(sht->buf, sht->bxsize, b, x, y, x + l * 8 - 1, y + 15); // 不超尾
	putfonts8_asc(sht->buf, sht->bxsize, x, y, c, s);
	sheet_refresh(sht, x, y, x + l * 8, y + 16); // 超尾
	return;
}

void putfont8_sht(struct SHEET *const sht, const int x, const int y, const int c, const int b, const char ch)
{
	/*
	在一个图层的某个位置打印字符。
	x,y: 坐标   c: 颜色   b: 背景颜色   ch: 字符
	*/
	boxfill8(sht->buf, sht->bxsize, b, x, y, x + 7, y + 15);	// 不超尾
	putfont8(sht->buf, sht->bxsize, x, y, c, font + ch * 16); // 间隔16个byte
	sheet_refresh(sht, x, y, x + 8, y + 16);									// 超尾
	return;
}

void make_wtitle8(unsigned char *const buf, const int xsize, char const *const title, char const act)
{
	/*
	在buf中画窗口的标题栏。
	 */
	const static char closebtn[14][16] =
			{
					"OOOOOOOOOOOOOOO@",
					"OQQQQQQQQQQQQQ$@",
					"OQQQQQQQQQQQQQ$@",
					"OQQQ@@QQQQ@@QQ$@",
					"OQQQQ@@QQ@@QQQ$@",
					"OQQQQQ@@@@QQQQ$@",
					"OQQQQQQ@@QQQQQ$@",
					"OQQQQQ@@@@QQQQ$@",
					"OQQQQ@@QQ@@QQQ$@",
					"OQQQ@@QQQQ@@QQ$@",
					"OQQQQQQQQQQQQQ$@",
					"OQQQQQQQQQQQQQ$@",
					"O$$$$$$$$$$$$$$@",
					"@@@@@@@@@@@@@@@@",
			};
	int x, y;
	char c, tc, tbc; // tc: 标题颜色 tbc：标题栏背景颜色
	if (act)
	{
		tc = white;
		tbc = darkblue;
	}
	else
	{
		tc = gray;
		tbc = darkgray;
	}
	boxfill8(buf, xsize, tbc, 3, 3, xsize - 4, 20);
	putfonts8_asc(buf, xsize, 24, 4, tc, title);
	for (y = 0; y < 14; y++)
		for (x = 0; x < 16; x++)
		{
			c = closebtn[y][x];
			if (c == '@')
				c = black;
			else if (c == '$')
				c = darkgray;
			else if (c == 'Q')
				c = gray;
			else
				c = white;
			buf[(5 + y) * xsize + (xsize - 21 + x)] = c;
		}
	return;
}

void make_window8(unsigned char *const buf, const int xsize, const int ysize, char const *const title, char const act)
{
	/*
	制造窗口的buf
	*/
	boxfill8(buf, xsize, gray, 0, 0, xsize - 1, 0);
	boxfill8(buf, xsize, white, 1, 1, xsize - 2, 1);
	boxfill8(buf, xsize, gray, 0, 0, 0, ysize - 1);
	boxfill8(buf, xsize, white, 1, 1, 1, ysize - 2);
	boxfill8(buf, xsize, darkgray, xsize - 2, 1, xsize - 2, ysize - 2);
	boxfill8(buf, xsize, black, xsize - 1, 0, xsize - 1, ysize - 1);
	boxfill8(buf, xsize, gray, 2, 2, xsize - 3, ysize - 3);
	boxfill8(buf, xsize, darkgray, 1, ysize - 2, xsize - 2, ysize - 2);
	boxfill8(buf, xsize, black, 0, ysize - 1, xsize - 1, ysize - 1);
	make_wtitle8(buf, xsize, title, act);
	return;
}

void make_textbox8(struct SHEET *const sht, const int x0, const int y0, const int sx, const int sy, const int c)
{
	/*
	在指定图层画一个背景色为c的文本框。
	*/
	int x1 = x0 + sx, y1 = y0 + sy;
	boxfill8(sht->buf, sht->bxsize, darkgray, x0 - 2, y0 - 3, x1 + 1, y0 - 3);
	boxfill8(sht->buf, sht->bxsize, darkgray, x0 - 3, y0 - 3, x0 - 3, y1 + 1);
	boxfill8(sht->buf, sht->bxsize, white, x0 - 3, y1 + 2, x1 + 1, y1 + 2);
	boxfill8(sht->buf, sht->bxsize, white, x1 + 2, y0 - 3, x1 + 2, y1 + 2);
	boxfill8(sht->buf, sht->bxsize, black, x0 - 1, y0 - 2, x1 + 0, y0 - 2);
	boxfill8(sht->buf, sht->bxsize, black, x0 - 2, y0 - 2, x0 - 2, y1 + 0);
	boxfill8(sht->buf, sht->bxsize, gray, x0 - 2, y1 + 1, x1 + 0, y1 + 1);
	boxfill8(sht->buf, sht->bxsize, gray, x1 + 1, y0 - 2, x1 + 1, y1 + 1);
	boxfill8(sht->buf, sht->bxsize, c, x0 - 1, y0 - 1, x1 + 0, y1 + 0);
	return;
}
