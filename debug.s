.section .text
.global debug_init
.global k_inb
.global k_outb
.global debug_print
.global k_strlen

.type k_inb, @function
k_inb:
	mov %rdi, %rdx
	xor %rax, %rax
	inb (%dx), %al
	ret
.size k_inb, .-k_inb

.type k_outb, @function
k_outb:
	mov %rdi, %rdx
	mov %rsi, %rax
	outb %al, %dx
	ret
.size k_outb, . - k_outb

.type debug_init, @function
debug_init:
	mov $(0x3f8 + 1), %rdx
	mov $0, %al
	outb %al, %dx
	mov $(0x3f8 + 3), %rdx
	mov $128, %rax
	outb %al, %dx
	mov $0x3f8, %rdx
	mov $3, %rax
	outb %al, %dx
	mov $(0x3f8 + 1), %rdx
	mov $0, %rax
	outb %al, %dx
	mov $(0x3f8 + 3), %rdx
	mov $3, %rax
	outb %al, %dx
	mov $(0x3f8 + 2), %rdx
	mov $199, %rax
	outb %al, %dx
	mov $(0x3f8 + 4), %rdx
	mov $11, %rax
	outb %al, %dx
	ret

.type debug_print, @function
debug_print:
	push %rdi
	mov $(0x3f8 + 5), %rdx
	xor %rax, %rax
1:
	inb %dx, %al
	test $0x20, %al
	jz 1b

	pop %rsi
	mov $0x3f8, %rdx
2:
	movb (%rsi), %al
	test %al, %al
	jz 3f
	outb %al, %dx
	inc %rsi
	jmp 2b

3:
	ret

.type k_strlen, @function
k_strlen:
	mov %rdi, %rcx
	xor %rax, %rax
1:
	cmpb $0, (%rcx)
	je 2f
	inc %rcx
	inc %rax
	jmp 1b
2:
	ret


