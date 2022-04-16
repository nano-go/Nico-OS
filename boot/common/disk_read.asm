; ------------------------------------------
; Function:
;		Read N sectors from the main disk	
; Params: 
;		CX    - sector count.
;   DI:SI - LBA Number
;   DS:BX - Dest Address
; ------------------------------------------
read_hard_disk_cx:
	push ebx

	.read_a_sector:
		call read_hard_disk_0
		inc si
		add ebx, 512
		loop .read_a_sector

	pop ebx
	ret



; ------------------------------------------
; Function:
;		Read one sector from the main disk	
; Params: 
;   DI:SI - LBA Number
;   DS:BX - Dest Address
; ------------------------------------------
read_hard_disk_0:
	
	push eax
	push ebx
	push ecx
	push edx

	; Set the number of sectors.
	mov dx, 0x1f2	
	mov al, 0x01           ; read one sector.
	out dx, al

	; Write LBA sector number to ports.
	inc dx                 ; 0x1f3
	mov ax, si
	out dx, al             ; LBA Addr 7～0

	inc dx                 ; 0x1f4
	mov al, ah
	out dx, al             ; LBA Addr 15～8

	inc dx                 ; 0x1f5
	mov ax, di
	out dx, al             ; LBA Addr 23～16

	inc dx                 ; 0x1f6
	mov al, 0B1110_0000    ; 0B1110 chooses LBA mode and main disk
	or al, ah
	out dx, al

	inc dx                 ; 0x1f7
	mov al, 0x20           ; Read Command
	out dx, al
	
	.waits:
		nop
		in al, dx
		and al, 0B10001000   ; Gets BYS And DRQ Bits
		cmp al, 0B00001000   ; BYS is 0 and DRQ is 1
		jne .waits
	
	mov cx, 256            ; read 256 words (512 bytes)
	mov dx, 0x1f0          ; from the data port

	.readw:
		in ax, dx
		mov word [ds:ebx], ax
		add bx, 2
		loop .readw
	
	pop edx
	pop ecx
	pop ebx
	pop eax
		
	ret
