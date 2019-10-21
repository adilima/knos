OUTPUT_FORMAT(elf64-x86-64)
ENTRY(bootstrap)

kernel_vma = 0xffffffff80000000;
kernel_lma = 0x100000;

SECTIONS
{
	. = kernel_lma;

	.bootstrap :
	{
		*(.multiboot)
		*(.bootstrap_text)
		*(.bootstrap_rodata*)
		*(.bootstrap_bss)
		*(.bootstrap_stack)
	}

	. += kernel_vma;

	.text : AT(ADDR(.text) - kernel_vma)
	{
		_code = .;
		*(.text)
		*(.rodata*)
		. = ALIGN(4K);
	}

	.data : AT(ADDR(.data) - kernel_vma)
	{
		_data = .;
		*(.data)
		. = ALIGN(4K);
	}

	.eh_frame : AT(ADDR(.eh_frame) - kernel_vma)
	{
		_ehframe = .;
		*(.eh_frame)
		. = ALIGN(4K);
	}

	.bss : AT(ADDR(.bss) - kernel_vma)
	{
		_bss = .;
		*(.bss)
		*(COMMON)
		. = ALIGN(4K);
	}

	_end = .;

	/DISCARD/ :
	{
		*(.comment)
	}
}

