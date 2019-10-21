.equiv MAGIC, 0xE85250D6
 
.section .multiboot
.global multiboot_header
.align 4
header_start:
	.long MAGIC
	.long 0
	.long header_end - header_start
	.long 0x100000000 - (MAGIC + 0 + (header_end - header_start))

.align 8
multiboot_header:

.align 8
end_tag:
	.short 0
	.short 0
	.long header_end - end_tag
header_end:


.section .bootstrap_stack, "aw", @nobits
.align 0x1000
stack_bottom:
	.skip 0x4000
stack_top:

.section .bootstrap_bss ,"aw", @nobits
.align 0x1000
paging_data_start:
pml4_table:
	.skip 0x1000
boot_pdp:
	.skip 0x1000
boot_page_directory:
	.skip 0x1000
kernel_heap:
	.skip 0x1000
kernel_pages:
	.skip 0x1000
kernel_temp_pages:
	.skip 0x1000
paging_data_end:
mbi_data:
	.skip 0x1000


.code32
.section .bootstrap_text
.global bootstrap
.type bootstrap, @function
.align 8
bootstrap:
	mov $stack_top, %esp
	mov %esp, %ebp

	call copy_mbi
	call amd64_setup

	ljmp $0x08, $_start_amd64

	cli
1:
	hlt
	jmp 1b

.align 8
amd64_setup:
	movl $boot_pdp, %eax
	orl  $0x3, %eax
	movl $pml4_table, %edi
	movl %eax, (%edi)
	movl $0, 4(%edi)

	movl $boot_page_directory, %eax
	orl  $0x3, %eax
	movl $boot_pdp, %edi
	movl %eax, (%edi)
	movl $0, 4(%edi)

####################################################
# We can choose to map the 1st 1 GB to itself
# from here, but since it is not recommended,
# then I guess we better follow the suggestions.
#
# Because we decided to load the kernel at 0x100000,
# then using only 2 pages is enough for temporary
# identity mapping.
####################################################
	mov $boot_page_directory, %edi
	mov $0, %esi
	mov $0, %ecx
1:
	mov %esi, %eax
	or  $0x83, %eax
	movl %eax, (%edi)
	movl $0, 4(%edi)
	add $0x200000, %esi
	add $8, %edi
	inc %ecx
	cmp $2, %ecx
	jne 1b

##########################################################
# Map Kernel Heap from 0xFFFFFFFF80000000 -> 0x0
#                                            1 GB
##########################################################
	movl $kernel_heap, %eax
	or  $0x3, %eax
	movl $(pml4_table + 511 * 8), %edi 
	movl %eax, (%edi)
	movl $0, 4(%edi)

	movl $kernel_pages, %eax
	or   $0x3, %eax
	movl $(kernel_heap + 510 * 8), %edi
	movl %eax, (%edi)
	movl $0, 4(%edi)

	movl $kernel_pages, %edi
	movl $0, %esi
	movl $0, %ecx
2:
	movl %esi, %eax
	orl  $0x83, %eax
	movl %eax, (%edi)
	movl $0, 4(%edi)
	add $8, %edi
	add $0x200000, %esi
	inc %ecx
	cmp $512, %ecx
	jne 2b

##########################################################
# Initialize a set of pages at even higher address space
# For kernel misc usage, but these are not initialized,
# the values are all 'not present'.
##########################################################
	movl $kernel_temp_pages, %eax
	or $0x3, %eax
	movl $(kernel_heap + 511 * 8), %edi
	movl %eax, (%edi)
	movl $0, 4(%edi)


	movl $pml4_table, %eax
	movl %eax, %cr3

	movl %cr4, %eax
	orl  $(1 << 5), %eax
	movl %eax, %cr4

	movl $0xc0000080, %ecx
	rdmsr
	or $(1 << 8), %eax
	wrmsr

#########################################
# The most critical state.
# We actually enable paging, after this
# we can get crashed if some code above
# miscalculated.
#########################################
	mov %cr0, %ecx
	or $(1 << 31), %ecx
	mov %ecx, %cr0

####################################################
# If everything was okay, then we should be able to
# load our gdt from here, then back to the caller,
# who will make the far jump into amd64 code.
####################################################
	lgdt gdt_ptr
	ret

.align 8
gdt64:
	.quad 0		# always 0
gdt_code:
	.short 0
	.short 0
	.byte 0
	.byte 0b10011000
	.byte 0b00100000
	.byte 0
gdt_data:
	.short 0
	.short 0
	.byte 0
	.byte 0b10010010
	.byte 0b00000000
	.byte 0
gdt_ptr:
	.short . - gdt64 - 1
	.quad gdt64


.align 8
copy_mbi:
	push %ebx
	mov $mbi_data, %edi
	mov %ebx, %eax
	movl (%eax), %ecx
	mov %eax, %esi
	repnz movsb

	xorl %eax, %eax
	movl $4, %ecx
	rep stosl
	pop %ebx
	ret

.code64
.extern kmain
.extern _bss
.extern kernel_vma
_start_amd64:
	mov $0x10, %ax
	mov %ax, %ss
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs

################################################################
# Initialize AMD64 Kernel Stack to the correct virtual address
# don't use old Bootstrap's BSS anymore.
################################################################
	movq $_bss, %rsp
	movq %rsp, %rbp

################################################################
# Point mbi_data to correct map location, so the kernel use
# correct addressing for all variables.
################################################################
	mov $mbi_data, %rdi
	add $kernel_vma, %rdi
	call kmain

	cli
1:
	hlt
	jmp 1b


