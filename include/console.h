
#pragma once


/* CONSOLE */
struct Console
{
	unsigned int current_start_addr;	/* 当前显示到了什么位置	  */
	unsigned int original_addr;		/* 当前控制台对应显存位置 */
	unsigned int v_mem_limit;		/* 当前控制台占的显存大小 */
	unsigned int cursor;			/* 当前光标位置 */
};

#define SCR_UP	1	/* scroll forward */
#define SCR_DN	-1	/* scroll backward */
#define SCR_SIZE		(80 * 25)
#define SCR_WIDTH		80

#define SCREEN_SIZE		(80 * 25)
#define SCREEN_WIDTH		80

#define DEFAULT_CHAR_COLOR	(MAKE_COLOR(BLACK, WHITE))
#define GRAY_CHAR		(MAKE_COLOR(BLACK, BLACK) | BRIGHT)
#define RED_CHAR		(MAKE_COLOR(BLUE, RED) | BRIGHT)

void out_char(Console* p_con, char ch);
void scroll_screen(Console* p_con, int direction);

int is_current_console(Console* p_con);
void select_console(int nr_console);
void scroll_screen(Console* p_con, int direction);
void set_cursor(unsigned int position);
void set_video_start_addr(u32 addr);
void flush(Console* p_con);
