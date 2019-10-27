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
	virt = k_memory_map(phys, 0xffffffffc0400000, map_size);
	if (!virt)
		debug_print("\n[system::framebuffer] \033[1;31mWARNING: Failed to map virtual address for framebuffer at 0xffffffffc0000000.\033[0m\n");
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
	const char *strTest = "-------------------- Framebuffer initialized --------------------";
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
 * Preparing to implement simple text scrolling using external buffer.
 * The text itself should be allocated as an array of 'rows' (char buffer),
 * separated from the buffer itself.
 *
 * Currently this function just copy every pixels from the src,
 * which is provided by main.cpp (as a plain rect filled with some colors).
 */
void system::framebuffer::blt(unsigned *buf, int xpos, int ypos, int nwidth, int nheight)
{
	unsigned *src = buf;
	unsigned *dest = nullptr;
	for (int y = ypos; y < (ypos + nheight); y++)
	{
		dest = (unsigned*)(virt + (y * pitch) + (xpos * 4));
		int lastX = nwidth;
		while (lastX)
		{
			*dest++ = *src++;
			lastX--;
		}
	}
}

