section .text
global getpid
$getpid: 
	mov eax, 0
	int 0x80
	ret

section .text
global write
$write: 
	mov eax, 1
	mov ebx, [esp + 4]
	mov ecx, [esp + 8]
	mov edx, [esp + 12]
	int 0x80
	ret

section .text
global read
$read: 
	mov eax, 2
	mov ebx, [esp + 4]
	mov ecx, [esp + 8]
	mov edx, [esp + 12]
	int 0x80
	ret

section .text
global open
$open: 
	mov eax, 3
	mov ebx, [esp + 4]
	mov ecx, [esp + 8]
	int 0x80
	ret

section .text
global stat
$stat: 
	mov eax, 4
	mov ebx, [esp + 4]
	mov ecx, [esp + 8]
	int 0x80
	ret

section .text
global close
$close: 
	mov eax, 5
	mov ebx, [esp + 4]
	int 0x80
	ret

section .text
global mkdir
$mkdir: 
	mov eax, 6
	mov ebx, [esp + 4]
	int 0x80
	ret

section .text
global unlink
$unlink: 
	mov eax, 7
	mov ebx, [esp + 4]
	int 0x80
	ret

section .text
global yield
$yield: 
	mov eax, 8
	int 0x80
	ret

section .text
global fork
$fork: 
	mov eax, 9
	int 0x80
	ret

section .text
global sbrk
$sbrk: 
	mov eax, 10
	mov ebx, [esp + 4]
	int 0x80
	ret

section .text
global execv
$execv: 
	mov eax, 11
	mov ebx, [esp + 4]
	mov ecx, [esp + 8]
	int 0x80
	ret

section .text
global exit
$exit: 
	mov eax, 12
	mov ebx, [esp + 4]
	int 0x80
	ret

section .text
global wait
$wait: 
	mov eax, 13
	mov ebx, [esp + 4]
	int 0x80
	ret

section .text
global chdir
$chdir: 
	mov eax, 14
	mov ebx, [esp + 4]
	int 0x80
	ret

section .text
global dup
$dup: 
	mov eax, 15
	mov ebx, [esp + 4]
	int 0x80
	ret

section .text
global pipe
$pipe: 
	mov eax, 16
	mov ebx, [esp + 4]
	int 0x80
	ret
