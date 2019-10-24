#include "system.h"
#include "multiboot2.h"

namespace system
{
	framebuffer *fbdev = nullptr;
}

using namespace system;

system::framebuffer::framebuffer(uintptr_t tag)
{
	multiboot_tag_framebuffer *fbtag = reinterpret_cast<multiboot_tag_framebuffer*>(tag);
	phys = fbtag->common.framebuffer_addr;
	width = fbtag->common.framebuffer_width;
	height = fbtag->common.framebuffer_height;
	pitch = fbtag->common.framebuffer_pitch;
	bpp = fbtag->common.framebuffer_bpp;
	forecolor = 0xffffff;
	backcolor = 0x1010ff; // some blue color

	size_t map_size = pitch * height;
	
	////////////////////////////////////////////////////////////////
	// currently map to 0xffffffffd0400000
	// but we can map to other address, as long as it is in 
	// page boundary, if not then the result can be unpredictable
	////////////////////////////////////////////////////////////////
	virt = k_memory_map(phys, 0xffffffffd0400000, map_size);
	if (!virt)
		debug_print("\n[system::framebuffer] \033[1;31mWARNING: Failed to map virtual address for framebuffer.\033[0m\n");
}

void system::framebuffer::clear()
{
	unsigned *ptr = (unsigned*)virt;
	size_t count = width * height;
	for (size_t i = 0; i < count; i++)
	{
		// clear the whole screen using backcolor
		*ptr++ = backcolor;
	}
}

/**
 * Draw PSF Font on linear framebuffer.
 * Credit goes to OSDEV community, especially the original writer.
 */
void system::framebuffer::putchar(char ch, int xpos, int ypos)
{
	int bytesPerRow = (font->width + 7) / 8;
	uchar *glyph = ((uchar*)font) + font->header_size +
		(ch > 0 && ch < font->num_glyph ? ch : 0) * font->bytes_per_glyph;

	unsigned nOffset = (ypos * pitch) + (xpos * 4);

	unsigned x, y, line, mask;
	for (y = 0; y < font->height; y++)
	{
		line = nOffset;
		mask = 1 << (font->width - 1);

		for (x = 0; x < font->width; x++)
		{
			*((unsigned*)(virt + line)) = (((int)*glyph) & mask) ? forecolor : backcolor;
			mask >>= 1;
			line += 4;
		}

		glyph += bytesPerRow;
		nOffset += pitch;
	}
}

/**
 * A silly test function,
 * just to show that we are alive.
 */
void system::framebuffer::show_test()
{
	const char *strTest = "--------------------------- Framebuffer initialized ---------------------------";
	char *p = const_cast<char*>(strTest);
	int lastX = (int)font->width;
	int lastY = (int)font->height;
	unsigned oldForecolor = forecolor;
	forecolor = 0xffff10;
	while (*p)
	{
		putchar(*p++, lastX, lastY);
		lastX += font->width;
	}
	forecolor = oldForecolor;
}


/**
 * Actually, we need to implement a simple text scrolling using text buffering
 * for each row (the size for each row can be smaller than fb->width, which will
 * also determined by the size of the buffer).
 *
 * But in the meantime, the following function is still necessary to output
 * at least a decent text on screen directly.
 */

void system::framebuffer::draw_string(const char *strText, int xpos, int ypos)
{
	char *p = const_cast<char*>(strText);
	int lastX = (int)font->width * xpos;
	int lastY = (int)font->height * ypos;

	while (*p)
	{
		if ((*p == '\n') || (*p == '\r'))
		{
			// avoid printing newline or carriage return
			// they actually are unprintable.
			lastX = (int)font->width;
			lastY += (int)font->height;
			p++;
			continue;
		}
		putchar(*p++, lastX, lastY);
		lastX += font->width;
	}
}

