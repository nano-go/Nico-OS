
section .text
global switch_to   ; void switch_to(prev, next)
switch_to:

	push ebp
	push ebx
	push esi
	push edi

	; eax = &prev->context
	mov eax, [esp+20]
	; prev->context = esp
	mov [eax], esp


	; eax = &next->context
	mov eax, [esp+24]
	; esp = next->context
	mov esp, [eax]

	pop edi
	pop esi
	pop ebx
	pop ebp
	ret
	
