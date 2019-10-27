.section .text
.global ps2_read
.global ps2_write

.type ps2_read, @function
ps2_read:
	mov $1000, %rcx
	mov $0x64, %rdx
1:
	xor %rax, %rax
	inb (%dx), %al
	test $0x01, %rax
	jnz 2f
	dec %rcx
	cmp $0, %rcx
	jne 1b

2:
	xor %rax, %rax
	mov $0x60, %rdx
	inb (%dx), %al
	ret
.size ps2_read, . - ps2_read


.type ps2_write, @function
ps2_write:
	mov $1000, %rcx
	mov $0x64, %rdx
1:
	xor %rax, %rax
	inb (%dx), %al
	test $2, %al
	jnz 2f
	dec %rcx
	cmp $0, %rcx
	jne 1b

2:
	mov %rdi, %rax
	mov $0x61, %rdx
	outb %al, (%dx)
	ret
.size ps2_write, . - ps2_write


