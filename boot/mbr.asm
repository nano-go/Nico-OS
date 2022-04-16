%include "config.asm"

section mbralign=16 vstart=0x7c00

	mov ax, cs
	mov ss, ax
	mov sp, 0x7c00

	xor ax, ax
	mov ds, ax
	mov es, ax

;=========== Clear Screen
	mov ax, 0x600
	mov bx, 0x700
	mov cx, 0
	mov dx, 0x184f
	int 0x10

;=========== Reset Cursor
	mov ax, 0x200
	xor bx, bx
	xor dx, dx
	int 0x10

;=========== Print Message
	push ds
	mov ax, 0x1301
	mov bx, 0x000f
	xor dx, dx
	mov cx, StartBootMsg_Length
	mov bp, StartBootMsg
	int 0x10
	pop ds

;=========== Load Setup.S

	xor di, di
	xor si, SETUP_LBA
	mov bx, SETUP_ADDRESS
	mov cx, SETUP_SECTORS
	call read_hard_disk_cx

;=========== Print Load OK.
	push ds
	mov ax, 0x1301
	mov bx, 0x000f
	mov dx, 0x0100
	mov cx, LoadSetupOK_Length
	mov bp, LoadSetupOK
	int 0x10
	pop ds

;=========== Goto setup.S
	mov ax, SETUP_ADDRESS
	jmp ax

%include "disk_read.asm"


JmpLab dw 0x0000
StartBootMsg db "Start Boot..."
StartBootMsg_Length equ ($-StartBootMsg)

LoadSetupOK db "Load setup.S OK!."
LoadSetupOK_Length equ ($-LoadSetupOK)

times 510-($-$$) db 0x00
                 db 0x55, 0xaa

