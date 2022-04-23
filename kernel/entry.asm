
%define SETUP_DATA_SELECTOR ((4<<3) | 0b000)
%define SYS_DATA_SELECTOR   ((2<<3) | 0b000)

extern kernel_start

[bits 32]
section .text.entry
global _start
_start:
;========== Copy a part of ".data section" in setup.S to ".data section" in this file.
	cld
	mov ax, SETUP_DATA_SELECTOR
	mov ds, ax
	mov ax, SYS_DATA_SELECTOR
	mov es, ax

	xor esi, esi
	mov edi, TOTAL_MEMORY
	mov ecx, 2
	rep movsw

	mov ax, SYS_DATA_SELECTOR
	mov ds, ax
	mov fs, ax

	call kernel_start
	jmp $

section .text
global get_total_memory ; uint32_t get_total_memory(void);
get_total_memory:
	mov eax, [TOTAL_MEMORY]
	ret

section .data
	TOTAL_MEMORY      dd 0x00000000
