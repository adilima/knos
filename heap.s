.section .text
.global heap_init

.type heap_init, @function
heap_init:
	movq $heap_root_start, %rax
	movq %rdi, (%rax)
	movq $heap_root_length, %rax
	movq %rsi, (%rax)                               # RSI = requested length
	movq (%rax), %rcx
	movq %rdi, %rax                                 # RAX = start of heap

#######################################################
# Calculate the limits, and round it to PAGE_SIZE
# where PAGE_SIZE = 4096
#
# Note:
# In this case heap_root_start = start of this struct
# should be equal to C/C++ style of heap* root;
# and heap_root_length is the first field, equivalent
# to what we define in system.h as k_object struct.
#######################################################
	add  $(heap_root_end - heap_root_length), %rax  # size of struct k_object
	add %rcx, %rax                                  # next_available = sizeof(struct k_object) + len
	add %rdi, %rax                                  # RAX = sizeof(k_object) + len + start
	movq $heap_next_available, %rcx
	test %rax, 0xFFFFFFFFFFFFF000
	jz 1f
	
	# round to page size
	and $0xFFFFFFFFFFFFF000, %rax
	add $0x1000, %rax
1:
	movq %rax, (%rcx)                              # heap_next_available = RAX
	sub $1, %rax
	movq $(heap_root_limits), %rdx
	movq %rax, (%rdx)                              # heap_root_limits = heap_next_available - 1

#######################################################
# Set the heap->data to start + sizeof(k_object)
#######################################################
	movq $(heap_root_end - heap_root_length), %rax    # sizeof(k_object)
	add %rdi, %rax                                    # RAX = start + sizeof(k_object)
	movq $heap_root_data, %rdx
	movq %rax, (%rdx)                                 # heap_root_data = start + sizeof(k_object)

#######################################################
# Initialize the next to nullptr.
#######################################################
	movq $heap_root_next, %rdx
	movq $0, (%rdx)

#######################################################
# RAX is already contain start + sizeof(k_object),
# so simply return the pointer
#######################################################
	ret

.section .bss
heap_root_start:
	.skip 8
heap_root_length:
	.skip 8
heap_root_limits:
	.skip 8
heap_root_data:
	.skip 8
heap_root_next:
	.skip 8
heap_root_end:
heap_next_available:
	.skip 8

