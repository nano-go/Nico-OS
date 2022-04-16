[bits 32]

extern general_trap
section .text
global alltraps_entry
alltraps_entry:
	push ds
	push es
	push fs
	push gs
	pushad

	; Output EOI to 8259A(main and slave).
	mov al, 0x20
	out 0xA0, al
	out 0x20, al

	push esp   ; Push trap stack frame.
	call general_trap
	add esp, 4  ; Pop trap stack frame.

	jmp exit_intr

global exit_intr
exit_intr:
	popad
	pop gs 
	pop fs
	pop es
	pop ds
	add esp, 8      ; Pop ErrorCode And VecNum
	iretd
