#include "system.h"


void k_strcpy(char *dest, const char *src)
{
	char *p1 = const_cast<char*>(src);
	char *p2 = dest;
	while (*p1)
		*p2++ = *p1++;
	*p2 = 0;
}

/**
 * If dest[0] == 0,
 * Then this will act the same as k_strcpy()
 */
void k_strcat(char *dest, const char *strText)
{
	char *p = &dest[k_strlen(dest)];
	k_strcpy(p, strText);
}

void k_strcat_l(char *dest, const char *title, size_t value)
{
	char *p = &dest[k_strlen(dest)];
	k_strcpy(p, title);
	p = &dest[k_strlen(dest)];
	char *p1 = p; // copy the address, we will use it later
	size_t ud = value;

	do
	{
		int remainder = ud % 10;
		*p++ = remainder + '0';
	} while (ud /= 10);

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
		strBuff->append("But currently we just display time from RTC.");

		char tmp[256];
		tmp[0] = 0;
		input_buf[0] = 0;

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

	text_buffer *terminal::find_line(int num)
	{
		text_buffer *retval = strBuff;
		while (retval)
		{
			if (retval->line_no == num)
				return retval;
			retval = retval->next;
		}
		return nullptr;
	}

	void terminal::draw_buffers()
	{
		text_buffer *pData = strBuff;

		int lastRow = font->height;
		int lastCol = font->width;
		int lineCount = 0;
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
			lineCount++;
			if (lineCount >= rows())
				break;
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

	int terminal::current_line_number()
	{
		text_buffer *tmp = strBuff;
		int n = 0;
		while (tmp->next)
		{
			tmp = tmp->next;
			++n;
		}
		return n;
	}

	void terminal::puts(const char *strText)
	{
		// discard the oldest row
		// if we exceed rows()
		if (current_line_number() >= rows())
		{
			text_buffer *tmp = strBuff;
			strBuff = tmp->next;
			DEBUG_PTR("[terminal]::puts() deleting oldest buffer => ", tmp);
			debug_size(" (line ", tmp->line_no);
			debug_print(")\n");
			delete tmp;
		}

		text_buffer *pNew = strBuff->append(strText);
		clear();
		draw_buffers();
	}
}

uint32_t k_get_time(void)
{
	uint8_t seconds;
	uint8_t minutes;
	uint8_t hours;

	/**
	 * Get the time from RTC
	 * we cannot get it all at once,
	 * since we must send the selected CMOS register number first,
	 * before we can get read from 0x71 port.
	 *
	 * We get value from port 0x71 is based on previously selected
	 * CMOS register number.
	 */
	k_outb(0x70, (1 << 7) | 0x04);
	hours = k_inb(0x71);
	k_outb(0x70, (1 << 7) | 0x02);
	minutes = k_inb(0x71);
	k_outb(0x70, (1 << 7) | 0x00);
	seconds = k_inb(0x71);
	return seconds << 16 | minutes << 8 | hours;
}

void system::terminal::test_input()
{
	char buf[128];
	uint8_t hour = k_get_rtc(RTC_HOURS);
	uint8_t minute = k_get_rtc(RTC_MINUTES);
	uint8_t secs = k_get_rtc(RTC_SECONDS);

	buf[0] = 0;
	k_strcat_x(buf, "Current Time ", hour);
	k_strcat_x(buf, ":", minute);
	k_strcat_x(buf, ":", secs);

	//////////////////////////////////////////
	// find the last line
	// don't create anymore text_buffer
	// we use the last line to avoid error.
	/////////////////////////////////////////
	text_buffer *prev = nullptr;
	text_buffer *tmp = strBuff;
	while (tmp->next)
	{
		prev = tmp;
		tmp = tmp->next;
	}
	
	k_strcpy(tmp->line, buf);

	int nchars = 0;
	while (nchars < 80)
	{
		char ch = system::getchar();
		if (ch < 0)
			break;
		buf[nchars++] = ch;
	}

	buf[nchars++] = 0;
	k_strcpy(prev->line, buf);

	clear();

	draw_buffers();
}

