#pragma once

#include <stddef.h>
#include <stdint.h>

typedef unsigned char  uchar;
typedef unsigned short ushort;
typedef unsigned long  ulong;
typedef unsigned long  k_addr_t;

#define HIGHLANDER_VERSION      0x03010100

extern "C" {

	/**
	 * Pulling in linker variable defined in linker script.
	 */
	void kernel_lma(void);
	void kernel_vma(void);
	void kernel_virt_end(void);

	/**
	 * These are implemented in assembly.
	 */

	void debug_init(void);
	void debug_print(const char* strText);

	void k_outb(uint16_t port, uint8_t value);
	uint8_t k_inb(uint16_t port);

	size_t k_strlen(const char *strText);
}

/**
 * Wait for serial transmit line to be ready.
 */
void debug_wait_ready(void);

/**
 * Convenient for outputing Hex and Integer values.
 */
void debug_addr(const char *strText, uintptr_t addr);

/**
 * If bAppendBytes = true, it will append text ' bytes'
 * after the integer number.
 * If intended for something else, just give false.
 * Default = false.
 */

void debug_size(const char *strText, size_t nsize, bool bAppendBytes=false);

/**
 * Wrapper, so we don't have to use typecast all the time
 */
#define DEBUG_PTR(title, val) \
	debug_addr(((const char *)(title)), ((uintptr_t)(val)))

uintptr_t k_memory_map(uintptr_t phys, uintptr_t virt, size_t len);

namespace system
{
	struct k_object
	{
		size_t length;
		uintptr_t data;
		uintptr_t limit;
		k_object  *next;
	};
	extern k_object *root;

	void *HeapAlloc(size_t len);
	void HeapFree(void *pv);

	class String
	{
		char *buffer;
		size_t length;
	public:
		String();
		String(size_t len);
		String(const char *src);
		virtual ~String();

		// Display the content on VGA Console
		void Show(uintptr_t pos);
		char *Data() const;

	protected:
		char *Dup(const char *src);
	};

	struct framebuffer
	{
		uintptr_t phys;
		uintptr_t virt;
		uint32_t width;
		uint32_t height;
		uint32_t pitch;
		uint32_t bpp;

		/**
		 * Some default colors
		 */
		uint32_t forecolor;
		uint32_t backcolor;

		/**
		 * Defined as uintptr_t
		 * because we don't want to pull in multiboot2.h
		 */
		framebuffer(uintptr_t tag);
		void clear();
	};

	/**
	 * There should be one and only fbdev all the way.
	 * And it should be declared somewhere in kernel's code.
	 */
	extern framebuffer *fbdev;
}

void *operator new(size_t len);
void *operator new[](size_t len);
void operator delete(void *pv) noexcept;
void operator delete[](void *pv) noexcept;
void operator delete(void *pv, size_t len) noexcept;
void operator delete[](void *pv, size_t len) noexcept;

