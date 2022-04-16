[bits 32]
section .text
extern main
extern exit
global _start
_start:
	push ecx
	push ebx
	call main
	add esp, 8
	push eax
	call exit
