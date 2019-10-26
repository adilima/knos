#pragma once

#include <stddef.h>
#include <stdint.h>

typedef unsigned char  uchar;
typedef unsigned short ushort;
typedef unsigned long  ulong;
typedef unsigned long  k_addr_t;  // unused so far, maybe deleted soon

#define RTC_SECONDS      0
#define RTC_MINUTES      2
#define RTC_HOURS        4
#define RTC_WEEKDAY      6
#define RTC_DAY_OF_MONTH 7
#define RTC_MONTH        8
#define RTC_YEAR         9
#define RTC_CENTURY      0x32

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

	/**
	 * Read RTC register.
	 * 0x04 = Hour
	 * 0x02 = Minute
	 * 0x00 = Seconds
	 * 0x06 = Weekday ( 1-7, 1 = Sunday )
	 * 0x07 = Day of Month (1 - 31)
	 * 0x08 = Month (1 - 12)
	 * 0x09 = Year  (0-99)
	 * 0x32 = Century (maybe) (19-20)
	 * 0x0A = Status Register A
	 * 0x0B = Status Register B
	 */
	uint8_t k_get_rtc(uint8_t rtc_id);

	size_t k_strlen(const char *strText);
	int k_str_equal(const char *str1, const char *str2, size_t len);
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
void k_strcpy(char *dest, const char *src);
uint32_t k_get_time(void);

struct PSF_FONT {
	uint32_t magic;
	uint32_t version;
	uint32_t header_size;
	uint32_t flags;
	uint32_t num_glyph;
	uint32_t bytes_per_glyph;
	uint32_t height;
	uint32_t width;
};

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
	char getchar();

	/**
	 * Simple String implementation
	 * Only for test.
	 */
	class String
	{
		char *buffer;
		size_t length;
		bool m_bInternal;
	public:
		String();
		String(size_t len);
		String(const char *src);
		virtual ~String();

		// Display the content on VGA Console
		void Show(uintptr_t pos);
		char *Data() const;

		void Copy(const char *strText);

		// display debug text on serial
		void Debug();
		void Append(const char *strText);
		void Append(const char *title, void *paddr);
		void AppendNumber(const char *title, size_t value);

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
		PSF_FONT *font;

		/**
		 * Defined as uintptr_t
		 * because we don't want to pull in multiboot2.h
		 */
		framebuffer(uintptr_t tag);
		void clear();
		void show_test();
		void putchar(char ch, int xpos, int ypos);
		void draw_string(const char *strText, int xpos, int ypos);

		/**
		 * The framebuffer does not need to know what the external buffer
		 * contains, it can be text or any other graphics primitives.
		 *
		 * It only copy the pixels to the specified location.
		 *
		 * Actually putchar() and draw_string() should be implemented
		 * in another class, representing a text buffer, then we can
		 * use this function to display the text here.
		 */
		void blt(unsigned *buf, int xpos, int ypos, int cx, int cy);
	};

	/**
	 * There should be one and only fbdev all the way.
	 * And it should be declared somewhere in kernel's code.
	 */
	extern framebuffer *fbdev;

	// forward decl
	struct text_buffer;

	class terminal
	{
		uint32_t *pixels;
		uint32_t width;
		uint32_t height;
		uint32_t forecolor;
		uint32_t backcolor;
		uint32_t pitch;
		PSF_FONT *font; // will copy the address from system::fbdev::font
		text_buffer *strBuff;
		char input_buf[80];

	public:
		terminal(uint32_t cx, uint32_t cy);
		~terminal();

		uint32_t *pixel_data() const;
		uint32_t window_width() const;
		uint32_t window_height() const;
		uint32_t rows();
		uint32_t columns();

		void clear();
		void puts(const char *strText);

		/**
		 * Loop until ESC key pressed
		 */
		void test_input();
		char getchar();

	private:
		// render the rows into the buffer
		void draw_buffers();
		void putchar(char ch, int xpos, int ypos);
		int current_line_number();
		text_buffer *find_line(int num);
	};
}

void *operator new(size_t len);
void *operator new[](size_t len);
void operator delete(void *pv) noexcept;
void operator delete[](void *pv) noexcept;
void operator delete(void *pv, size_t len) noexcept;
void operator delete[](void *pv, size_t len) noexcept;

