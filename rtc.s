	.section .text
	.global k_get_rtc
	.global k_rtc_update_in_progress
	.global k_rtc_get_value
	
/**
 * Please note that, currently k_get_rtc()
 * does not check whether the value in RDI is valid.
 *
 * So the caller should ensure that RDI contains
 * a valid RTC register value, otherwise this function
 * may return unpredictable value.
 *
 * Also note that, NMI is currently disabled by each call
 * so if it was enabled, then you must re-enable it after
 * calling this function.
 */

	.type k_get_rtc, @function
k_get_rtc:
	# RDI = RTC register
	mov $(1 << 7), %rax
	or %rdi, %rax
	mov $0x70, %rdx
	outb %al, (%dx)
	xor %rax, %rax
	mov $0x71, %rdx
	inb (%dx), %al
	ret
	.size k_get_rtc, . - k_get_rtc

	.type k_rtc_update_in_progress, @function
k_rtc_update_in_progress:
	mov $0x0a, %rax
	mov $0x70, %rdx
	outb %al, (%dx)
	mov $0x71, %rdx
	xor %rax, %rax
	inb (%dx), %al
	and $0x80, %al
	ret
	.size k_rtc_update_in_progress, . - k_rtc_update_in_progress


	.type k_rtc_get_value, @function
k_rtc_get_value:
	mov $0x70, %rdx
	mov %rdi, %rax
	outb %al, (%dx)
	mov $0x71, %rdx
	xor %rax, %rax
	inb (%dx), %al
	ret
	.size k_rtc_get_value, . - k_rtc_get_value
	
	
