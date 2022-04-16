%define	SYS_execv 11
%define SYS_exit  12

[bits 32]
section .text vstart=0x4048000
start:
	mov eax, SYS_execv
	mov ebx, initpath
	mov ecx, argv
	int 0x80

exit:
	mov eax, SYS_exit
	int 0x80
	jmp exit

initpath:
	db "/bin/init", 0
init:
	db "init", 0

argv:
	dd init
	dd 0
