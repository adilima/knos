#include "system.h"

static
void ps2_wait(void)
{
	uint8_t val = 0;
	for (int i = 0; i < 1000; i++)
	{
		val = k_inb(0x64);
		if (val & 1)
			return;
	}
}

static
uint8_t ps2_read(void)
{
	ps2_wait();
	return k_inb(0x60);
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
			{ 16, 'Q' }, { 17, 'W' }, { 18, 'E' }, { 19, 'R' },
			{ 20, 'T' }, { 21, 'Y' }, { 22, 'U' }, { 23, 'I' },
			{ 24, 'O' }, { 25, 'P' }, { 30, 'A' }, { 31, 'S' },
			{ 32, 'D' }, { 33, 'F' }, { 34, 'G' }, { 35, 'H' },
			{ 36, 'J' }, { 37, 'K' }, { 38, 'L' }, { 44, 'Z' },
			{ 45, 'X' }, { 46, 'C' }, { 47, 'V' }, { 48, 'B' },
			{ 49, 'N' }, { 50, 'M' }, { 51, '<' }, { 52, '>' },
			{ 53, '?' }, { 39, ':' }, { 40, '\"' }, { 43, '|' }
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
			{ 16, 'q' }, { 17, 'w' }, { 18, 'e' }, { 19, 'r' },
			{ 20, 't' }, { 21, 'y' }, { 22, 'u' }, { 23, 'i' },
			{ 24, 'o' }, { 25, 'p' }, { 30, 'a' }, { 31, 's' },
			{ 32, 'd' }, { 33, 'f' }, { 34, 'g' }, { 35, 'h' },
			{ 36, 'j' }, { 37, 'k' }, { 38, 'l' }, { 44, 'z' },
			{ 45, 'x' }, { 46, 'c' }, { 47, 'v' }, { 48, 'b' },
			{ 49, 'n' }, { 50, 'm' }, { 51, ',' }, { 52, '.' },
			{ 53, '/' }, { 39, ';' }, { 40, '\'' }, { 43, '\\' }
		};

		for (int i = 0; i < (sizeof(lcase_chars) / sizeof(lcase_chars[0])); i++)
		{
			if (sc == lcase_chars[i].scancode)
				return lcase_chars[i].char_val;
		}
	}

	return -1;
}

#define INVALID_KEY 0xFA

static
void k_keyboard_write(uchar sc)
{
	ps2_wait();
	k_outb(0x61, sc);
}

char system::getchar(void)
{
	// display keyboard input, if any...
	uint8_t sc = ps2_read();
	k_keyboard_write(sc);

	if (sc == INVALID_KEY)
		return -1;

	if ((sc == 42) || (sc == 54))
	{
		// shift just down
		uchar next_key = ps2_read();
		k_keyboard_write(next_key);
		ps2_read();

		uchar retval = scancode_to_char(next_key, true);
		if (retval != -1)
			return retval;
	}
	else
	{
		// no shift, just lcase (or unprintable char, or symbols).
		// I think we must read again from 0x60
		ps2_read();
		uchar retval = scancode_to_char(sc, false);
		if (retval != -1)
			return retval;
	}

	// return invalid char
	return -1;
}

