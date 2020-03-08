
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                               tty.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "type.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "interruption.h"
#include "assert.h"

#include "keyboard.h"

using namespace proc;

#define TTY_FIRST	(tty_table)
#define TTY_END		(tty_table + NR_CONSOLES)

u8 sys_getchar(u32 u1, u32 u2, u32 u3, Process* p_proc);

int		disp_pos;

int		nr_current_console;

Tty		tty_table[NR_CONSOLES];

Console		console_table[NR_CONSOLES];

static void init_tty(Tty* p_tty);
static void tty_do_read(Tty* p_tty);
static void tty_do_write(Tty* p_tty);
static void put_key(Tty* p_tty, u32 key);



/*======================================================================*
                              RingBuffer
*======================================================================*/
static u32 ring_length(RingBuffer* rb)
{
    return (rb->head + RingBufferSize - rb->tail) % RingBufferSize;
}

u32 get_shell_rb()
{
    return ring_length(&tty_table[2].gb);
}

static void init_ring_buffer(RingBuffer* rb)
{
    rb->head = rb->tail = 0;
}

static void back_key(RingBuffer* rb)
{
    if (ring_length(rb) == 0)  return;
    rb->head = (rb->head - 1 + RingBufferSize) % RingBufferSize;
    return;
}

static u8 pop_key(RingBuffer* rb)
{
    if (ring_length(rb) == 0)  return '\0';
    u8 ch = rb->buffer[rb->tail];
    rb->tail = (rb->tail + 1) % RingBufferSize;
    return ch;
}

static void push_key(RingBuffer* rb, u8 ch)
{
    if (ring_length(rb) == (RingBufferSize - 1)) return;
    rb->buffer[rb->head] = ch;
    rb->head = (rb->head + 1) % RingBufferSize;
}

u8 sys_getchar(u32 u1, u32 u2, u32 u3, Process* p_proc)
{
    if (ring_length(&tty_table[p_proc->nr_tty].gb)) {return pop_key(&tty_table[p_proc->nr_tty].gb);}
    return '\0';
}

/*======================================================================*
                           task_tty
 *======================================================================*/
 void task_tty()
{
	Tty*	p_tty;

	init_keyboard();

	for (p_tty=TTY_FIRST;p_tty<TTY_END;p_tty++) {
		init_tty(p_tty);
	}
	select_console(0);
	while (1) {
		for (p_tty=TTY_FIRST;p_tty<TTY_END;p_tty++) {
			tty_do_read(p_tty);
			tty_do_write(p_tty);
		}
	}
}

/*======================================================================*
			   init_tty
 *======================================================================*/
static void init_tty(Tty* p_tty)
{
	p_tty->inbuf_count = 0;
	p_tty->p_inbuf_head = p_tty->p_inbuf_tail = p_tty->in_buf;
    init_ring_buffer(&p_tty->rb);
    init_ring_buffer(&p_tty->gb);
	init_screen(p_tty);
}

/*======================================================================*
				in_Process
 *======================================================================*/
 void in_process(Tty* p_tty, u32 key)
{
        char output[2] = {'\0', '\0'};

        if (!(key & FLAG_EXT)) {
		    put_key(p_tty, key);
            push_key(&p_tty->rb, key);
        }
        else {
            int raw_code = key & MASK_RAW;
            switch(raw_code) {
            case ENTER:
			    put_key(p_tty, '\n');
                push_key(&p_tty->rb, '\n');
                while (ring_length(&p_tty->rb)) push_key(&p_tty->gb, pop_key(&p_tty->rb));
			    break;
            case BACKSPACE:
			    put_key(p_tty, '\b');
                back_key(&p_tty->rb);
			    break;
            case UP:
                if ((key & FLAG_SHIFT_L) || (key & FLAG_SHIFT_R)) {
			        scroll_screen(p_tty->p_console, SCR_DN);
                }
			    break;
		    case DOWN:
		    	if ((key & FLAG_SHIFT_L) || (key & FLAG_SHIFT_R)) {
		    		scroll_screen(p_tty->p_console, SCR_UP);
		    	}
		    	break;
		    case F1:
		    case F2:
		    case F3:
		    case F4:
		    case F5:
		    case F6:
		    case F7:
		    case F8:
		    case F9:
		    case F10:
		    case F11:
		    case F12:
		    	/* Alt + F1~F12 */
		    	if ((key & FLAG_ALT_L) || (key & FLAG_ALT_R)) {
		    		select_console(raw_code - F1);
		    	}
		    	break;
            default:
                break;
            }
        }
}

/*======================================================================*
			      put_key
*======================================================================*/
static void put_key(Tty* p_tty, u32 key)
{
	if (p_tty->inbuf_count < TTY_IN_BYTES) {
		*(p_tty->p_inbuf_head) = key;
		p_tty->p_inbuf_head++;
		if (p_tty->p_inbuf_head == p_tty->in_buf + TTY_IN_BYTES) {
			p_tty->p_inbuf_head = p_tty->in_buf;
		}
		p_tty->inbuf_count++;
	}
}


/*======================================================================*
			      tty_do_read
 *======================================================================*/
static void tty_do_read(Tty* p_tty)
{
	if (is_current_console(p_tty->p_console)) {
		keyboard_read(p_tty);
	}
}


/*======================================================================*
			      tty_do_write
 *======================================================================*/
static void tty_do_write(Tty* p_tty)
{
	if (p_tty->inbuf_count) {
		char ch = *(p_tty->p_inbuf_tail);
		p_tty->p_inbuf_tail++;
		if (p_tty->p_inbuf_tail == p_tty->in_buf + TTY_IN_BYTES) {
			p_tty->p_inbuf_tail = p_tty->in_buf;
		}
		p_tty->inbuf_count--;

		out_char(p_tty->p_console, ch);
	}
}

/*======================================================================*
                              tty_write
*======================================================================*/
void tty_write(Tty* p_tty, const char* buf)
{
        while (*buf != '\0') {
            out_char(p_tty->p_console, *buf++);
        }
}

/*======================================================================*
                              sys_write
*======================================================================*/
int sys_write(u32 u1, u32 u2, char* s, Process* p_proc)
{
	char * p;
	char ch;

	char reenter_err[] = "? k_reenter is incorrect for unknown reason";
	reenter_err[0] = MAG_CH_PANIC;

	/**
	 * @note Code in both Ring 0 and Ring 1~3 may invoke printx().
	 * If this happens in Ring 0, no linear-physical address mapping
	 * is needed.
	 *
	 * @attention The value of `k_reenter' is tricky here. When
	 *   -# printx() is called in Ring 0
	 *      - k_reenter > 0. When code in Ring 0 calls printx(),
	 *        an `interrupt re-enter' will occur (printx() generates
	 *        a software interrupt). Thus `k_reenter' will be increased
	 *        by `kernel.asm::save' and be greater than 0.
	 *   -# printx() is called in Ring 1~3
	 *      - k_reenter == 0.
	 */
	if (k_reenter == 0)  /* printx() called in Ring<1~3> */
		p = (char*)va2la(p_proc - proc_table, (void *)s);
	else if (k_reenter > 0) /* printx() called in Ring<0> */
		p = s;
	else	/* this should NOT happen */
		p = reenter_err;

	/**
	 * @note if assertion fails in any TASK, the system will be halted;
	 * if it fails in a USER PROC, it'll return like any normal syscall
	 * does.
	 */
	if ((*p == MAG_CH_PANIC) ||
	    (*p == MAG_CH_ASSERT && p_proc_ready < &proc_table[proc_num])) {
		disable_int();
		char * v = (char*)V_MEM_BASE;
		const char * q = p + 1; /* +1: skip the magic char */

		while (v < (char*)(V_MEM_BASE + V_MEM_SIZE)) {
			*v++ = *q++;
			*v++ = RED_CHAR;
			if (!*q) {
				while (((int)v - V_MEM_BASE) % (SCR_WIDTH * 16)) {
					/* *v++ = ' '; */
					v++;
					*v++ = GRAY_CHAR;
				}
				q = p + 1;
			}
		}

		__asm__ __volatile__("hlt");
	}

	while ((ch = *p++) != 0) {
		if (ch == MAG_CH_PANIC || ch == MAG_CH_ASSERT)
			continue; /* skip the magic char */

		out_char(tty_table[p_proc->nr_tty].p_console, ch);
	}

	return 0;
}

/*======================================================================*
                               itoa
 *======================================================================*/
char * itoa(char * str, int num)/* 数字前面的 0 不被显示出来, 比如 0000B800 被显示成 B800 */
{
	char *	p = str;
	char	ch;
	int	i;
	int	flag = 0;

	*p++ = '0';
	*p++ = 'x';

	if(num == 0){
		*p++ = '0';
	}
	else{
		for(i=28;i>=0;i-=4){
			ch = (num >> i) & 0xF;
			if(flag || (ch > 0)){
				flag = 1;
				ch += '0';
				if(ch > '9'){
					ch += 7;
				}
				*p++ = ch;
			}
		}
	}

	*p = 0;

	return str;
}


/*======================================================================*
                               disp_int
 *======================================================================*/
void disp_int(int input)
{
	char output[16];
	itoa(output, input);
	disp_str(output);
}


void init_screen(Tty* p_tty)
{
	int nr_tty = p_tty - tty_table;
	p_tty->p_console = console_table + nr_tty;

	int v_mem_size = V_MEM_SIZE >> 1;	/* 显存总大小 (in WORD) */

	int con_v_mem_size                   = v_mem_size / NR_CONSOLES;
	p_tty->p_console->original_addr      = nr_tty * con_v_mem_size;
	p_tty->p_console->v_mem_limit        = con_v_mem_size;
	p_tty->p_console->current_start_addr = p_tty->p_console->original_addr;

	/* 默认光标位置在最开始处 */
	p_tty->p_console->cursor = p_tty->p_console->original_addr;

	if (nr_tty == 0) {
		/* 第一个控制台沿用原来的光标位置 */
		p_tty->p_console->cursor = disp_pos / 2;
		disp_pos = 0;
	}
	else {
		out_char(p_tty->p_console, nr_tty + '0');
		out_char(p_tty->p_console, '#');
	}

	set_cursor(p_tty->p_console->cursor);
}