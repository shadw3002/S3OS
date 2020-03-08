#pragma once

#include "type.h"
#include "const.h"

void add_ldt_desc(u32 i);

void init_idt();

void init_tss_desc();

/* 中断向量 */
#define	INT_VECTOR_DIVIDE		    0x0
#define	INT_VECTOR_DEBUG		    0x1
#define	INT_VECTOR_NMI			    0x2
#define	INT_VECTOR_BREAKPOINT		0x3
#define	INT_VECTOR_OVERFLOW		    0x4
#define	INT_VECTOR_BOUNDS		    0x5
#define	INT_VECTOR_INVAL_OP		    0x6
#define	INT_VECTOR_COPROC_NOT		0x7
#define	INT_VECTOR_DOUBLE_FAULT		0x8
#define	INT_VECTOR_COPROC_SEG		0x9
#define	INT_VECTOR_INVAL_TSS		0xA
#define	INT_VECTOR_SEG_NOT		    0xB
#define	INT_VECTOR_STACK_FAULT		0xC
#define	INT_VECTOR_PROTECTION		0xD
#define	INT_VECTOR_PAGE_FAULT		0xE
#define	INT_VECTOR_COPROC_ERR		0x10

#define	INT_VECTOR_IRQ0			    0x20
#define	INT_VECTOR_IRQ8			    0x28
/* 系统调用 */
#define INT_VECTOR_SYS_CALL         0x90

extern u32		k_reenter;
extern irq_handler	irq_table[NR_IRQ];

extern "C" {
    void sys_call();             /* int_handler */

    void disable_irq(int irq);
    void enable_irq(int irq);
    void disable_int();
    void enable_int();
    void put_irq_handler(int irq, irq_handler handler);
    void spurious_irq(int irq);

    void exception_handler(int vec_no, int err_code, int eip, int cs, int eflags);

    void divide_error();
    void single_step_exception();
    void nmi();
    void breakpoint_exception();
    void overflow();
    void bounds_check();
    void inval_opcode();
    void copr_not_available();
    void double_fault();
    void copr_seg_overrun();
    void inval_tss();
    void segment_not_present();
    void stack_exception();
    void general_protection();
    void page_fault();
    void copr_error();
    void hwint00();
    void hwint01();
    void hwint02();
    void hwint03();
    void hwint04();
    void hwint05();
    void hwint06();
    void hwint07();
    void hwint08();
    void hwint09();
    void hwint10();
    void hwint11();
    void hwint12();
    void hwint13();
    void hwint14();
    void hwint15();
}




