%include "config.asm"

section code align=16 vstart=SETUP_ADDRESS

	mov ax, section.data.start+SETUP_ADDRESS
	shr ax, 4
	mov ds, ax
	mov es, ax

;========== Set Display Mode
	; 80*25 16 bits color text mode.
	mov ax, 0x0003
	int 0x10

;========== Print SetupStart Message
	mov ax, 0x1301
	mov bx, 0x000f
	mov dx, 0x0200
	mov cx, StartSetupMsg_Length
	mov bp, StartSetupMsg
	int 0x10

;========== Protection Mode
	call detect_memory
	call load_gdt

	; Enable A20 addr wire.
	in al, 0x92
	or al, 0000_0010B
	out 0x92, al

	cli

	; Enable protection mode.
	mov eax, cr0
	or eax, 1
	mov cr0, eax

	jmp dword SYS_CODE_SELECTOR:pmode_start

[bits 32]
pmode_start:
	mov ax, SYS_STACK_SELECTOR
	mov ss, ax
	mov esp, SYS_STACK_POINTER

	mov ax, SYS_DATA_SELECTOR
	mov ds, ax
	mov es, ax

	mov ax, VIDEO_SELECTOR
	mov gs, ax


;========== Set up pages.
	call setup_page

	mov eax, PAGE_TABLE_BASE_ADDR
	mov cr3, eax
	mov eax, cr0
	or eax, 0x80000000
	mov cr0, eax

;========== Load kernel to the memory
	mov cx, KERNEL_SECTORS
	xor di, di
	mov si, KERNEL_LBA
	mov ebx, KERNEL_ADDRESS
	.load_head_loop:
		call read_hard_disk_0
		inc si
		add ebx, 512
		loop .load_head_loop

;========== Jump to the kernel entry.
	mov esp, SYS_STACK_POINTER + KERNEL_BASE
	jmp SYS_CODE_SELECTOR:KERNEL_ADDRESS









[bits 32]
%include "disk_read.asm"

;================================================================
; Function
;     detect_memory
;
; Description
;     Detect the memory and get the total amount of the memory.
;================================================================
[bits 16]
detect_memory:
	; @1: Try to use 0x15(eax=0xe820) interrupt to detect the memory.
	xor ebx, ebx
	mov edx, 0x534d4150        ; "SMAP"
	mov di, ARDS_BUF

	.detect_memory_loop1:
		mov eax, 0xe820
		; an ARDS is 20 bytes.
		mov ecx, 20
		int 0x15

		; 0x15(eax=0xe820) failed. Try to use 0x15(eax=0xe801) to detect the memory.
		jc detect_memory_0x15_0xe801

		; Move to the next ARDS ptr.
		add di, cx
		inc word [ARDS_COUNT]
		cmp ebx, 0
		jne .detect_memory_loop1
	
	; Use the EAX to store the maximum memory.
	xor eax, eax
	xor ecx, ecx
	mov cx, [ARDS_COUNT]
	mov ebx, ARDS_BUF
	.find_maximum_memarea:
		mov edx, [ebx]       ; low base addr
		add edx, [ebx+8]     ; low length
		add ebx, 20
		cmp edx, eax
		jle .next0           ; if edx <= eax
		mov eax, edx         ; edx > eax, update eax.
		.next0:
			loop .find_maximum_memarea

	jmp memget_ok	

detect_memory_0x15_0xe801:
	mov ax, 0xe801
	int 0x15
	; 0x15(eax=0xe801) failed. Try to use 0x15(ah=0x88) to detect the memory.
	jc detect_memory_0x15_0x88

	; Converts KB into Bytes: DX:AX *= 1024
	mov cx, 1024
	mul cx

	; high 16 bits is stored in the DX
	; low 16 bits is stored in the AX

	; Concat DX, AX to EAX
	shl edx, 16
	and eax, 0x0000FFFF
	or eax, edx

	; Extra 1MB memory, so EAX += 1MB
	add eax, 1024*1024*1

	; Save the result to ESI
	mov esi, eax

	; 64KB -> Bytes
	xor eax, eax
	mov ax, bx
	mov ecx, 1024*64
	mul ecx

	; Use EAX resgiter to store the maximum memory.
	add eax, esi
	jmp memget_ok

detect_memory_0x15_0x88:
	mov ah, 0x88
	int 0x15
	jc error_hlt


	; KB -> Bytes.
	; dx:ax *= 1024
	mov cx, 1024*1
	mul cx

	and eax, 0x0000FFFF
	shl edx, 16
	or eax, edx

	add eax, 1024*1024*1
	jmp memget_ok

error_hlt:
	hlt

memget_ok:
	mov [TOTAL_MEMORY], eax
	ret


;=============================================================
; Function
;     load_gdt
;
; Description
;     Load the global description table.
;=============================================================
[bits 16]
load_gdt:
	mov bx, GDT_BASE
	
	mov dword [ds:bx], 0x00000000
	mov dword [ds:bx+4], 0x00000000

	add bx, 8
	mov esi, 0x0000
	mov ecx, 0xfffff
	mov edx, GDT_ATTR_SYS_CODE
	call write_descriptor

	add bx, 8
	mov esi, 0x0000
	mov ecx, 0xfffff
	mov edx, GDT_ATTR_SYS_DATA
	call write_descriptor

	add bx, 8
	mov esi, 0x000b8000 | KERNEL_BASE
	mov ecx, 0xffff
	mov edx, GDT_ATTR_BYTE_DATA
	call write_descriptor

	add bx, 8
	mov esi, ds
	shl esi, 4
	mov ecx, 0xffff
	mov edx, GDT_ATTR_BYTE_DATA
	call write_descriptor

	mov word [ds:GDT_POINTER], 5*8-1
	mov eax, ds
	shl eax, 4
	add eax, GDT_BASE
	mov dword [ds:GDT_POINTER+2], eax
	lgdt [ds:GDT_POINTER]
	ret


;=============================================================
; Function
;     write_descriptor
;
; Description
;     Write a descriptor to GDT.
;
; Params
;     ESI     - segment base addr.
;     ECX     - segment bound.
;     EDX     - descriptor flags.
;     DS:BX   - gdt entry address.
;=============================================================
[bits 16]
write_descriptor:

	; Move Segment Limit(0-15) to EAX(0-15)
	mov eax, ecx
	and eax, 0x0000FFFF

	; Move Base Addr(0-15) to EAX(16-31)
	mov edi, esi
	and edi, 0x0000FFFF
	shl edi, 16
	or eax, edi
	mov dword [ds:bx], eax

	xor eax, eax
	xor edi, edi
	; Move Base Addr(16-23) to EAX(0-7)
	mov eax, esi
	and eax, 0x00FF0000
	shr eax, 16
	
	; Move Segment Limit(16-19) to EAX(16-19)
	mov edi, ecx
	and edi, 0x000F0000
	or eax, edi

	; Move Base Addr(24-31) to EAX(24-31)
	and esi, 0xFF000000
	or eax, esi

	; Move Flags to EAX
	or eax, edx
	mov dword [ds:bx+4], eax

	ret

;=============================================================
; Function
;     setup_page
;
; Description
;     Setup the page directory table(V->P):
;       0x00000000-0x00040000 -> 0x00000000-0x00040000 (4MB, Temp)
;       0x08000000-0x08040000 -> 0x00000000-0x00040000 (4MB)
;
; Memory Layout
;     Name            Address                    Memory Space
;     -------------------------------------------------------
;     PDT             PAGE_TABLE_BASE_ADDR         4 KB
;     Page Table1     PAGE_TABLE_BASE_ADDR + 4K    4 KB  
;================ ============================================
[bits 32]
setup_page:
	mov ebx, PAGE_TABLE_BASE_ADDR
	mov esi, 0
	mov ecx, 1024

	.clear_pgdir:
		mov dword [ds:PAGE_TABLE_BASE_ADDR+esi], 0
		add esi, 4
		loop .clear_pgdir

	; PDE is a pointer to the first page table.
	mov eax, (PAGE_TABLE_BASE_ADDR + 4096) | PG_RW_RW | PG_PRESENT | PG_US_SUPER

	mov [ds:PAGE_TABLE_BASE_ADDR], eax
	mov [ds:PAGE_TABLE_BASE_ADDR+0x800], eax

	mov ecx, 1024
	mov ebx, PAGE_TABLE_BASE_ADDR + 4096
	mov esi, PG_RW_RW | PG_PRESENT | PG_US_SUPER
	.create_pgtab:
		mov [ds:ebx], esi
		add esi, 4096
		add ebx, 4
		loop .create_pgtab

	ret
	

section data align=16 vstart=0
	TOTAL_MEMORY dd 0x00000000
	GDT_BASE:
		resb 128
	GDT_POINTER  dw 0x0000
               dd 0x00000000

	StartSetupMsg db "Setup Start..."
	StartSetupMsg_Length equ ($-StartSetupMsg)

	; Address Range Descriptor Structure array.
	ARDS_BUF times 512 db 0x00
	ARDS_COUNT dw 0x00
