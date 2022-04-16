VIDEO_SELECTOR equ (0b011<<3) | 0B000

CHAR_STYLE_ATTR    equ 0B0_000_1111
;---------------------------------------------------
; This file provides two functions:
;		put_char(char):
;			Print a char at the current cursor.
;		set_cursor(uint16_t):
;			Set the position of the cursor.
;---------------------------------------------------


[bits 32]
global put_char   ; void put_char(char)
put_char:
	pushad

	; Param: ascii char.
	mov byte cl, [esp+36]

	mov ax, VIDEO_SELECTOR
	mov gs, ax


	; Get the high 8 bits of the cursor position.
	mov dx, 0x3d4
	mov al, 0x0e
	out dx, al
	mov dx, 0x3d5
	in al, dx
	mov ah, al

	; Get the low 8 bits of the cursor position.
	mov dx, 0x3d4
	mov al, 0x0f
	out dx, al
	mov dx, 0x3d5
	in al, dx

	mov bx, ax            ; Save the cursor position to BX.


	cmp cl, 0x0a              ; Is Line Feed char?
	je .put_linefeed
	cmp cl, 0x0d              ; Is carriage return?
	je .put_linefeed
	cmp cl, 0x8               ; Is Backspace?
	jz .put_backspace

	jmp .put_other

	.put_linefeed:
		; cursor_position / 80 * 80
		mov ax, bx
		mov bl, 80
		div bl
		mul bl
		add ax, 80              ; Move the cursor to the next line.
		mov bx, ax
		jmp .roll_screen
	
	.put_backspace:
		dec bx
		shl bx, 1                 ; bx *= 2
		mov word [gs:bx], 0x0720  ; Replace the current char with the whitespace char.

		shr bx, 1
		jmp .roll_screen


	.put_other:
		shl bx, 1            ; bx *= 2
		mov byte [gs:bx], cl
		mov byte [gs:bx+1], CHAR_STYLE_ATTR

		shr bx, 1
		add bx, 1
	
	.roll_screen:
		cmp bx, 2000         ; If the cursor position gt 2000
		                     ; Out of full screen
		jl .set_cursor       ; No.

		; Roll the screen


		; Move [1-24] lines to [0-23] lines.
		; cld
		mov esi, 0x800b8000+(1*80*2)       ; Line 2 And Col 1
		mov edi, 0x800b8000                ; Line 1 And Col 1
		mov ecx, 24*80
		rep movsw

		; Clear the last line.
		mov ebx, 0x800b8000+(24*80*2)      ; Last line offset
		mov ecx, 80
		.clear_last_line:
			; 0x720 is white space.
			mov word [ebx], 0x0720
			add ebx, 2
			loop .clear_last_line

		mov bx, 24 * 80      ; Move the cursor to the start of last line.

	.set_cursor:
		mov dx, 0x3d4
		mov al, 0x0e
		out dx, al
		mov dx, 0x3d5
		mov al, bh
		out dx, al

		mov dx, 0x3d4
		mov al, 0x0f
		out dx, al
		mov dx, 0x3d5
		mov al, bl
		out dx, al

	popad
	ret



global set_cursor   ; void set_cursor(uint16_t)
set_cursor:
	push eax
	push ebx
	push edx

	mov word bx, [esp+16]

	mov dx, 0x3d4
	mov al, 0x0e
	out dx, al
	mov dx, 0x3d5
	mov al, bh
	out dx, al

	mov dx, 0x3d4
	mov al, 0x0f
	out dx, al
	mov dx, 0x3d5
	mov al, bl
	out dx, al

	pop edx
	pop ebx
	pop eax
	ret
