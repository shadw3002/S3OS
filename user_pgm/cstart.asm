
extern start
INT_VECTOR_SYS_CALL equ 0x90
_NR_write           equ 0
_NR_getch           equ 1

bits 32

[SECTION .data]

[SECTION .text]
global	write
global  getch

global _start


_start:
    call start
    hlt

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
