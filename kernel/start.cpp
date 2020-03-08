#include "type.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "hd_driver.h"
#include "systask.h"
#include "filesystem.h"
#include "interruption.h"
#include "clock.h"
#include "keyboard.h"

extern "C" {
    void cstart();
    int kernel_main();


}

static void add_tasks()
{
    task::add_sys_task(task_sys, 0x4000, "sys_task");
    task::add_sys_task(task_tty, 0x4000, "tty");
    task::add_sys_task(task_hd, 0x4000, "hard disk driver");
    task::add_sys_task(task_fs, 0x4000, "file system");

	task::add_user_task(TestA, 0x4000, "TestA");
	task::add_user_task(TestB, 0x4000, "TestB");
	task::add_user_task(shell_main, 0x4000, "shell");
}

static void change_gdt_ptr()
{
	// 将 LOADER 中的 GDT 复制到新的 GDT 中
	memcpy(	&gdt,				    // New GDT
		(void*)(*((u32*)(&gdt_ptr[2]))),   // Base  of Old GDT
		*((u16*)(&gdt_ptr[0])) + 1	    // Limit of Old GDT
		);
	// gdt_ptr[6] 共 6 个字节：0~15:Limit  16~47:Base。用作 sgdt 以及 lgdt 的参数。
	u16* p_gdt_limit = (u16*)(&gdt_ptr[0]);
	u32* p_gdt_base  = (u32*)(&gdt_ptr[2]);
	*p_gdt_limit = GDT_SIZE * sizeof(Descriptor) - 1;
	*p_gdt_base  = (u32)&gdt;
}

static void change_idt_ptr()
{
	// idt_ptr[6] 共 6 个字节：0~15:Limit  16~47:Base。用作 sidt 以及 lidt 的参数。
	u16* p_idt_limit = (u16*)(&idt_ptr[0]);
	u32* p_idt_base  = (u32*)(&idt_ptr[2]);
	*p_idt_limit = IDT_SIZE * sizeof(Gate) - 1;
	*p_idt_base  = (u32)&idt;
}

/*
_start:
	; 把 esp 从 LOADER 挪到 KERNEL
	mov	esp, StackTop	; 堆栈在 bss 段中

	mov	dword [disp_pos], 0

	sgdt	[gdt_ptr]	; cstart() 中将会用到 gdt_ptr
*/
void cstart()
{
    change_gdt_ptr();

    change_idt_ptr();

    proc::init_proc_table();

    task::init_task();

    add_tasks();

    init_idt();

    init_tss_desc();

	disp_str("-----\"cstart\" finished-----\n");
}
/*
	lgdt	[gdt_ptr]	; 使用新的GDT

	lidt	[idt_ptr]

	jmp	SELECTOR_KERNEL_CS:csinit
csinit:		; “这个跳转指令强制使用刚刚初始化的结构”——<<OS:D&I 2nd>> P90.

	;jmp 0x40:0
	;ud2


	xor	eax, eax
	mov	ax, SELECTOR_TSS
	ltr	ax

	;sti
*/
int kernel_main()
{
	disp_str("-----\"kernel_main\" begins-----\n");


	k_reenter = 0;

	clock::init();
    init_keyboard();

    p_proc_ready = proc::proc_table;

	restart();

	while(1){}
}