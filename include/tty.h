#pragma once

#include "proc.h"
#include "type.h"
#include "console.h"

#define TTY_IN_BYTES	256	/* tty input queue size */


#define RingBufferSize  256

struct RingBuffer
{
    u8 buffer[RingBufferSize];

    u32 head;
    u32 tail;
};

/* TTY */
struct Tty
{
	u32	in_buf[TTY_IN_BYTES];	/* TTY 输入缓冲区 */
	u32*	p_inbuf_head;		/* 指向缓冲区中下一个空闲位置 */
	u32*	p_inbuf_tail;		/* 指向键盘任务应处理的键值 */
	int	inbuf_count;		/* 缓冲区中已经填充了多少 */

	Console*	p_console;

    RingBuffer rb;
    RingBuffer gb;
};

u8 sys_getch(u32 a, u32 b, proc::Process* p_proc);
u32 get_shell_rb();

struct s_console;

extern int		nr_current_console;

extern Tty		tty_table[NR_CONSOLES];

extern Console		console_table[NR_CONSOLES];

extern int		disp_pos;

extern "C" {
    void disp_str(char * info);

    void disp_color_str(char * info, int color);
}

char * itoa(char * str, int num);

void disp_int(int input);

void task_tty();
void in_process(Tty* p_tty, u32 key);
void init_screen(Tty* p_tty);