
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
;                               syscall.asm
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
;                                                     Forrest Yu, 2005
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

%include "sconst.inc"

INT_VECTOR_SYS_CALL equ 0x90
_NR_write           equ 0
_NR_getch           equ 1
_NR_sendrec         equ 2
_NR_fork            equ 3
_NR_get_pid         equ 4

; 导出符号
global	write
global  getch
global  sendrec
global  fork
global  get_pid

bits 32
[section .text]

; ====================================================================================
;                          void write(char* buf);
; ====================================================================================
write:
        mov     eax, _NR_write
        mov     edx, [esp + 4]
        int     INT_VECTOR_SYS_CALL
        ret

; ====================================================================================
;                          u8 getch();
; ====================================================================================
getch:
        mov     eax, _NR_getch

        int     INT_VECTOR_SYS_CALL
        ret

; ====================================================================================
;                  sendrec(int function, int src_dest, MESSAGE* msg);
; ====================================================================================
; Never call sendrec() directly, call send_recv() instead.
sendrec:
	mov	eax, _NR_sendrec
	mov	ebx, [esp + 4]	; function
	mov	ecx, [esp + 8]	; src_dest
	mov	edx, [esp + 12]	; p_msg
	int	INT_VECTOR_SYS_CALL
	ret

; ====================================================================================
;                          void    fork();
; ====================================================================================
fork:
        mov     eax, _NR_fork

        int     INT_VECTOR_SYS_CALL
        ret

; ====================================================================================
;                          u32     get_pid();
; ====================================================================================
get_pid:
        mov     eax, _NR_get_pid

        int     INT_VECTOR_SYS_CALL
        ret