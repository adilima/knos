#include "system.h"


void k_strcpy(char *dest, const char *src)
{
	char *p1 = const_cast<char*>(src);
	char *p2 = dest;
	while (*p1)
		*p2++ = *p1++;
	*p2 = 0;
}

void k_strcat(char *dest, const char *strText)
{
	char *p = &dest[k_strlen(dest)];
	k_strcpy(p, strText);
}

void k_strcat_x(char *dest, const char *title, uintptr_t value)
{
	char *p = &dest[k_strlen(dest)];
	k_strcpy(p, title);
	p = &dest[k_strlen(dest)];
	char *p1 = p; // copy the address, we will use it later
	size_t ud = value;

	do
	{
		int remainder = ud % 16;
		*p++ = (remainder < 10) ? remainder + '0' : remainder + 'A' - 10;
	} while (ud /= 16);

	*p = 0; // terminate the string
	char *p2 = p - 1;
	while (p1 < p2)
	{
		char tmp = *p1;
		*p1 = *p2;
		*p2 = tmp;
		p1++;
		p2--;
	}
}

namespace system
{
	// define a struct for internal use
	struct text_buffer
	{
		/* fixed 80 * 2 chars max */
		char line[160];
		int line_no;
		int columns;
		text_buffer *next;
		text_buffer()
		{
			line[0] = 0;
			line_no = 0;
			columns = 80;
			next = nullptr;
		}

		text_buffer(int nCols)
		{
			line[0] = 0;
			line_no = 0;
			columns = nCols;
			if (nCols > 160)
			{
				nCols = 160;
				debug_size("[text_buffer] WARNING: column count ", nCols);
				debug_print(" is not supported, maximum is 160, truncated to 160.\n");
			}
			next = nullptr;
		}

		text_buffer *append(const char *strText)
		{
			text_buffer *prev = this;
			while (prev->next)
				prev = prev->next;
			text_buffer* retval = new text_buffer(columns);
			retval->line_no = prev->line_no + 1;
			k_strcpy(retval->line, strText);
			prev->next = retval;
			return retval;
		}
	};

	terminal::terminal(uint32_t cx, uint32_t cy)
		: width(cx),
		height(cy)
	{
		pixels = new uint32_t[cx * cy];
		pitch = cx * 4;
		font = fbdev->font;
		forecolor = 0x808080;
		backcolor = 0x101010;
		strBuff = new text_buffer(cx / font->width);
		k_strcpy(strBuff->line, "KNOS Console");
		strBuff->append("The next text outputs should be drawn here, instead of the left panel.");
		strBuff->append("We will implement text scrolling soon.");

		char tmp[256];
		tmp[0] = 0;
		k_strcat_x(tmp, "This terminal object has address => 0x", (uintptr_t)this);
		strBuff->append(tmp);

		clear();
		draw_buffers();
	}

	terminal::~terminal()
	{
		if (pixels)
		{
			delete[] pixels;
			pixels = nullptr;
		}
	}

	void terminal::clear()
	{
		unsigned *p = pixels;
		for (unsigned i = 0; i < height; i++)
			for (unsigned j = 0; j < width; j++)
				*p++ = backcolor;
	}

	uint32_t *terminal::pixel_data() const
	{
		return *(uint32_t**)&pixels;
	}

	uint32_t terminal::window_width() const
	{
		return *(uint32_t*)&width;
	}

	uint32_t terminal::window_height() const
	{
		return *(uint32_t*)&height;
	}

	uint32_t terminal::rows()
	{
		return height / font->height;
	}

	uint32_t terminal::columns()
	{
		return width / font->width;
	}

	void terminal::draw_buffers()
	{
		text_buffer *pData = strBuff;
		int lastRow = font->height;
		int lastCol = font->width;
		while (pData)
		{
			char *pchar = pData->line;
			while (*pchar)
			{
				putchar(*pchar++, lastCol, lastRow);
				lastCol += font->width;
			}

			pData = pData->next;
			lastCol = font->width;
			lastRow += font->height;
		}
	}

	void terminal::putchar(char ch, int xpos, int ypos)
	{
		int bytesPerRow = (font->width + 7) / 8;
		uchar* glyph = ((uchar*)font) + font->header_size +
			(ch > 0 && ch < font->num_glyph ? ch : 0) * font->bytes_per_glyph;
		unsigned nOffset = (ypos * pitch) + (xpos * 4);

		unsigned l, m;

		// convert the pixels pointer into integer
		// otherwise the incoming calculations will be wrong
		// :-)
		uintptr_t uptr = (uintptr_t)pixels;
		for (unsigned y = 0; y < font->height; y++)
		{
			l = nOffset;
			m = 1 << (font->width - 1);
			for (unsigned x = 0; x < font->width; x++)
			{
				*((unsigned*)(uptr + l)) = (((int)*glyph) & m) ? forecolor : backcolor;
				m >>= 1;
				l += 4;
			}
			glyph += bytesPerRow;
			nOffset += pitch;
		}
	}
	
	void terminal::puts(const char *strText)
	{
	    strBuff->append(strText);
	    
	    // we have to find a way to display the 'visible' rows only
	    // and create scrolling effects.
	    // currently, just redraw all
	    clear();
	    draw_buffers();
	}
}

