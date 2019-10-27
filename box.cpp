#include "system.h"

void k_strcpy(char *dest, const char *src)
{
	char *p2 = const_cast<char*>(src);
	char *p1 = dest;
	while (*p2)
		*p1++ = *p2++;
	*p1 = 0;
}

// immediate decl
namespace system
{
	namespace drawing
	{
		void fill_rect(unsigned *buffer, int x, int y, int cx, int cy, unsigned ncolor);
	}
}

using namespace system;

box::box()
{
	m_pFont = fbdev->font;
	k_strcpy(m_szTitle, "KNOS Silly Terminal - Test");
	m_nRows = 5;
	m_nWidth = 80 * m_pFont->width;
	m_nHeight = 6 * m_pFont->height;  // add 1 for the title

	// unsafe to create context inside this struct
	m_pContext = new uint32_t[m_nWidth * m_nHeight];
	
	m_nLength = 400;          // 80 * 5 chars, including null terminating char
	m_bExternalData = false;  // always try to use internal storage space, if possible

	m_forecolor = 0x808080;
	m_backcolor = 0x101010;   // not really black, but a nice looking dark

	////////////////////////////////////////////////////////////////////////////////////////////
	// Now, we must check if we have enough room in the internal page allocated by HeapAlloc()
	// So that we won't have to allocate a new storage.
	////////////////////////////////////////////////////////////////////////////////////////////
	k_object *container = reinterpret_cast<k_object*>(((uintptr_t)this) - sizeof(k_object));
	if (container->data != (uintptr_t)this)
	{
		m_bExternalData = true;
		m_pszText = new char[m_nLength];
		debug_size("[box] WARNING: allocated ", m_nLength);
		debug_print(" bytes external storage for text buffer.\nYou should allocate this object on the heap though.\n");
	}
	else
	{
		// Currently there's no way to create non-default size of box
		// So we don't bother checking the bounds, but we MUST in near future,
		// when we want it to be more flexible.
		uintptr_t data_location = container->data + sizeof(box);
		m_pszText = (char*)data_location;
		m_pszText[0] = 0;
		container->length += m_nLength;
	}
	draw_title();
	debug_print("[box] ready to go...\n");
}

box::~box()
{
	if (m_pszText)
	{
		if (m_bExternalData)
		{
			delete[] m_pszText;
			m_pszText = nullptr;
		}
	}
	if (m_pContext)
	{
		delete[] m_pContext;
		m_pContext = nullptr;
	}
}

size_t box::text_length()
{
	return k_strlen(m_pszText);
}

void box::set_text(const char *pszText)
{
	size_t len = k_strlen(pszText);
	if (len >= m_nLength)
	{
		debug_size("[box]::set_text() WARNING: the text length exceeded buffer length -> ", len);
		debug_print(", truncated.\n");

		// do copying locally
		// k_strcpyn() not yet implemented
		len = m_nLength - 1;
		char *pszDest = m_pszText;
		char *pszSrc  = const_cast<char*>(pszText);
		while (len)
		{
			*pszDest++ = *pszSrc++;
			--len;
		}
		*pszDest = 0;
	}
	else
		k_strcpy(m_pszText, pszText);
	
	// render the text into graphical array of glyphs
	render_text();
}

void box::render_text()
{
	system::drawing::fill_rect(m_pContext,
			0, m_pFont->height,
			m_nWidth,
			m_nHeight - m_pFont->height,
			m_backcolor);
	
	int lastX = 0;
	int lastY = m_pFont->height;  // start at row 1
	char *p = m_pszText;

	while (*p)
	{
		if ((*p == '\n') || (*p == '\r'))
		{
			// skip this char by adding a newline
			// but not drawing it.
			p++;
			lastX = 0;
			lastY += m_pFont->height;
			continue;
		}
		putchar(*p++, lastX, lastY);
		lastX += m_pFont->width;
		if (lastX >= m_nWidth)
		{
			lastX = 0;
			lastY += m_pFont->height;
		}
	}
}

void box::draw_title()
{
	system::drawing::fill_rect(m_pContext, 
			0, 0, 
			m_nWidth,
			m_pFont->height,
			0xc0c0c0);

	// save current state
	unsigned saveForecolor = m_forecolor;
	unsigned saveBackcolor = m_backcolor;
	m_backcolor = 0xc0c0c0;
	m_forecolor = 0x101010;

	int lastX = m_pFont->width;
	char *p = m_szTitle;
	while (*p)
	{
		putchar(*p++, lastX, 0);
		lastX += m_pFont->width;   // it should not exceed 80 chars (I dont bother checking)
	}

	m_backcolor = saveBackcolor;
	m_forecolor = saveForecolor;
}

void system::drawing::fill_rect(unsigned *buffer,
		int x, int y,
		int cx, int cy,
		unsigned ncolor)
{
	int nPitch = cx * 4;

	for (int i = y; i < (y + cy); i++)
	{
		uintptr_t location = ((uintptr_t)buffer) + (i * nPitch);
		for (int j = x; j < (x + cx); j++)
		{
			*(unsigned*)(location + (j * 4)) = ncolor;
		}
	}
}

void box::show(int xpos, int ypos)
{
	fbdev->blt(m_pContext,
			xpos, ypos,
			m_nWidth,
			m_nHeight);
}

void box::putchar(char c, int xpos, int ypos)
{
	int nPitch = (m_pFont->width + 7) / 8;
	char *glyph = ((char*)m_pFont) + m_pFont->header_size + 
		(c > 0 && c < m_pFont->num_glyph ? c : 0) * m_pFont->bytes_per_glyph;

	unsigned nOffset = (ypos * m_nWidth * 4) + (xpos * 4);
	int line, mask;

	uintptr_t location = (uintptr_t)m_pContext;
	for (int y = 0; y < m_pFont->height; y++)
	{
		line = nOffset;
		mask = 1 << (m_pFont->width - 1);

		for (int x = 0; x < m_pFont->width; x++)
		{
			*((unsigned*)(location + line)) = (((int)*glyph) & mask) ? m_forecolor : m_backcolor;
			mask >>= 1;
			line += 4;
		}

		glyph += nPitch;
		nOffset += (m_nWidth * 4);
	}
}

void box::set_title(const char *strTitle)
{
    k_strcpy(m_szTitle, strTitle);
    draw_title();
}

