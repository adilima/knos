#include "system.h"

static
void ps2_wait()
{
	uchar status = 0;
	for (int i = 0; i < 1000; i++)
	{
		status = k_inb(0x64);
		if (status & 1)
			break;
	}
}

static
uchar scancode_to_char(uchar sc, bool bShift)
{
	if (bShift)
	{
		struct ucase {
			uchar scancode;
			uchar char_val;
		} ucase_chars[] = {
			{ 2, '!' }, { 3, '@' }, { 4, '#' }, { 5, '$' },
			{ 6, '%' }, { 7, '^' }, { 8, '&' }, { 9, '*' },
			{ 10, '(' }, { 11, ')' }, { 12, '_' }, { 13, '+' },
			{ 15, '\t' }, { 26, '{' }, { 27, '}' },
			{ 16, 'Q' }, { 17, 'W' }, { 18, 'E' }, { 19, 'R' },
			{ 20, 'T' }, { 21, 'Y' }, { 22, 'U' }, { 23, 'I' },
			{ 24, 'O' }, { 25, 'P' }, { 30, 'A' }, { 31, 'S' },
			{ 32, 'D' }, { 33, 'F' }, { 34, 'G' }, { 35, 'H' },
			{ 36, 'J' }, { 37, 'K' }, { 38, 'L' }, { 44, 'Z' },
			{ 45, 'X' }, { 46, 'C' }, { 47, 'V' }, { 48, 'B' },
			{ 49, 'N' }, { 50, 'M' }, { 51, '<' }, { 52, '>' },
			{ 53, '?' }, { 39, ':' }, { 40, '\"' }, { 43, '|' },
			{ KEY_SPACE, ' ' }, { KEY_ENTER, '\n' },
		};

		for (int i = 0; i < (sizeof(ucase_chars) / sizeof(ucase_chars[0])); i++)
		{
			if (sc == ucase_chars[i].scancode)
				return ucase_chars[i].char_val;
		}
		return -1;
	}
	else
	{
		struct lcase {
			uchar scancode;
			uchar char_val;
		} lcase_chars[] = {
			{ 2, '1' }, { 3, '2' }, { 4, '3' }, { 5, '4' },
			{ 6, '5' }, { 7, '6' }, { 8, '7' }, { 9, '8' },
			{ 10, '9' }, { 11, '0' }, { 12, '-' }, { 13, '=' },
			{ 15, '\t' }, { 26, '[' }, { 27, ']' },
			{ 16, 'q' }, { 17, 'w' }, { 18, 'e' }, { 19, 'r' },
			{ 20, 't' }, { 21, 'y' }, { 22, 'u' }, { 23, 'i' },
			{ 24, 'o' }, { 25, 'p' }, { 30, 'a' }, { 31, 's' },
			{ 32, 'd' }, { 33, 'f' }, { 34, 'g' }, { 35, 'h' },
			{ 36, 'j' }, { 37, 'k' }, { 38, 'l' }, { 44, 'z' },
			{ 45, 'x' }, { 46, 'c' }, { 47, 'v' }, { 48, 'b' },
			{ 49, 'n' }, { 50, 'm' }, { 51, ',' }, { 52, '.' },
			{ 53, '/' }, { 39, ';' }, { 40, '\'' }, { 43, '\\' },
			{ KEY_SPACE, ' ' }, { KEY_ENTER, '\n' },
		};

		for (int i = 0; i < (sizeof(lcase_chars) / sizeof(lcase_chars[0])); i++)
		{
			if (sc == lcase_chars[i].scancode)
				return lcase_chars[i].char_val;
		}
	}

	return -1;
}

char system::getchar()
{
	uchar sc = ps2_read();
	if (sc == 0xFA)
		sc = ps2_read();

	if (sc == ACK)
		sc = ps2_read();

	// ps2_write(sc);

	if ((sc == KEY_LSHIFT) || (sc == KEY_RSHIFT))
	{
		// Mustinya sih tidak ada 0x80 ya...
		// karena ini kan awalnya, sepanjang tidak dilepas, maka
		// next char pasti adalah targetnya.
		debug_print("[system]::getchar() SHIFT Down\n");
		uchar key = ps2_read();
		while (key)
		{
			if (key & 0x80) break;
			key = ps2_read();
		}

		ps2_write(key);

		key &= ~0x80;
		uchar retval = scancode_to_char(key, true);
		if (key == (sc + 0x80))
		{
			debug_print("[system]::getchar() SHIFT Up\n");
			return 0;
		}
		if (retval != -1)
			return retval;
	}
	else
	{
		/**
		 * Discard the bytes until
		 * we receive KeyUp event
		 */
		uchar key = ps2_read();
		while (key)
		{
			if (key & 0x80) break;
			key = k_inb(0x60);   // does it have any effect?
		}

		// is it right? sending 0xfa
		ps2_write(ACK);
		
		uchar retval = scancode_to_char(sc, false);
		if (retval != -1)
			return retval;
	}

	return -1;
}

