#pragma once

#include <stddef.h>
#include <stdint.h>

typedef unsigned char  uchar;
typedef unsigned short ushort;
typedef unsigned long  ulong;
typedef unsigned long  k_addr_t;

#define RTC_SECONDS      0x00
#define RTC_MINUTES      0x02
#define RTC_HOURS        0x04
#define RTC_WEEKDAY      0x06
#define RTC_DAY_OF_MONTH 0x07
#define RTC_MONTH        0x08
#define RTC_YEAR         0x09
#define RTC_CENTURY      0x32


#define ACK 0xFA
#define KEY_ENTER 28
#define KEY_LSHIFT 42
#define KEY_RSHIFT 54
#define KEY_SPACE 57
#define KEY_BACKSPACE 14

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
	int k_str_equal(const char *str1, const char *str2, size_t len);

	/**
	 * For dealing with PS2
	 */
	uint8_t ps2_read(void);
	void ps2_write(uint8_t value);
	
	uint8_t k_get_rtc(uint8_t index);
	
	/**
	 * Return non-zero if RTC update is in progress
	 */
	int k_rtc_update_in_progress(void);
	uint8_t k_rtc_get_value(uint8_t index);
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

char *k_strcat(char *p1, const char *p2);
char *k_strcat_x(char *p1, const char *title, uintptr_t value);
char *k_strcat_l(char *p1, const char *title, size_t value);

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
	struct k_object;
	struct framebuffer;
	class String;
	class box;

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
		void blt(unsigned *buf, int xpos, int ypos, int nwidth, int nheight);
	};

	/**
	 * There should be one and only fbdev all the way.
	 * And it should be declared somewhere in kernel's code.
	 */
	extern framebuffer *fbdev;

	/**
	 * Easiest way is to define these items
	 * in system namespace, thus avoid possible
	 * collisions in the future.
	 */
	char getchar();

	class box
	{
		//////////////////////////////////////////////////////////////////
		// too many objects using width, height, etc
		// we better prefix internal private vars with m_
		// add p for pointers.
		//
		// Yeaah I know, it's Microsoft notions, but it's not a bad idea.
		//////////////////////////////////////////////////////////////////
		uint32_t *m_pContext; // context for drawing
		int m_nWidth;
		int m_nHeight;
		int m_nRows;          // how many rows this buffer has?
		char *m_pszText;
		size_t m_nLength;       // length of the text buffer
		bool m_bExternalData;
		PSF_FONT *m_pFont;   // a copy of fbdev->font
		char m_szTitle[80];  // fixed length string for title of the box
		unsigned m_forecolor;
		unsigned m_backcolor;

	public:
        ////////////////////////////////////////////////////////
		// will create a default 80 chars width and 5 rows box
		// using default fore/backcolors.
		// Actually 6 rows (including the title box).
		////////////////////////////////////////////////////////
		box();
		~box();

		int width() const
		{ return *(int*)&m_nWidth; }
		int height() const
		{ return *(int*)&m_nHeight; }
		uint32_t *data() const
		{ return *(uint32_t**)&m_pContext; }
		char *title() const
		{ return *(char**)&m_szTitle; }
		size_t text_length();
		void set_text(const char *pszText);
		
		void set_title(const char *strTitle);

        // display the box at the specified location
        // in the framebuffer
		void show(int xpos, int ypos);

	protected:
		void draw_title();
		void render_text();
		void putchar(char c, int xpos, int ypos);
	};
}

void *operator new(size_t len);
void *operator new[](size_t len);
void operator delete(void *pv) noexcept;
void operator delete[](void *pv) noexcept;
void operator delete(void *pv, size_t len) noexcept;
void operator delete[](void *pv, size_t len) noexcept;

