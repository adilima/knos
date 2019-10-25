#include "system.h"


using namespace system;

system::String::String()
{
	m_bInternal = false;
	k_object *container = reinterpret_cast<k_object*>(((uintptr_t)this) - sizeof(k_object));
	if (container->data != (uintptr_t)this)
	{
		buffer = new char[256];
		length = 256;
		DEBUG_PTR("\n[system::String] allocated external storage of 256 bytes at ", buffer);
	}
	else
	{
		// this object was created on the heap
		// we should use it's allocated page instead of creating new one.
		buffer = (char*)(container->data + sizeof(String));
		size_t max_length = container->limit - (uintptr_t)buffer;
		length = (max_length < 256) ? max_length - 1 : 256;
		DEBUG_PTR("\n[system::String] using internal storage at ", buffer);
		debug_size(" with max length = ", length);
		debug_print(" bytes.\n");
		m_bInternal = true;
	}
}

system::String::String(size_t len)
{
	buffer = new char[len];
	length = len;
	m_bInternal = false;
}

system::String::String(const char *src)
{
	buffer = nullptr;
	Dup(src);
	// length was automaticaly set by Dup()
}

char *system::String::Dup(const char *src)
{
	m_bInternal = false;
	k_object *container = reinterpret_cast<k_object*>(((uintptr_t)this) - sizeof(k_object));
	if (container->data != (uintptr_t)this)
	{
		if (buffer == nullptr)
		{
			buffer = new char[256];
			length = 256;
			DEBUG_PTR("\n[system::String] allocated external storage of 256 bytes at ", buffer);
		}
		else
		{
			if (length < k_strlen(src))
			{
				delete[] buffer;
				buffer = new char[k_strlen(src) + 1];
				length = k_strlen(src) + 1;
			}
		}
	}
	else
	{
		// this object was created on the heap
		// we should use it's allocated page instead of creating new one.
		buffer = (char*)(container->data + sizeof(String));
		size_t max_length = container->limit - (uintptr_t)buffer;
		length = (max_length < 256) ? max_length - 1 : 256;
		DEBUG_PTR("\n[system::String] using internal storage at ", buffer);
		debug_size(" with max length = ", length);
		debug_print(" bytes.\n");
		m_bInternal = true;
	}

	char *p = buffer;
	char *p2 = const_cast<char*>(src);
	while (*p2)
		*p++ = *p2++;
	*p = 0;
	return buffer;
}

/**
 * We need to modify the following function soon,
 * to be able to output to EFI/VBE Framebuffer,
 * instead of VGA.
 */
void system::String::Show(uintptr_t pos)
{
	char *video = (char*)pos;
	char *p = buffer;
	while (*p)
	{
		video[0] = *p++;
		video[1] = 0x3f;
		video += 2;
	}
}

system::String::~String()
{
	if (buffer)
	{
		if (!m_bInternal)
		{
			DEBUG_PTR("[sytem::String] deleting buffer => ", buffer);
			delete [] buffer;
			buffer = nullptr;
			length = 0;
		}
		// if using internal storage, then we do nothing
	}
	DEBUG_PTR("\n[system::String(", this);
	debug_print(")] terminating.\n");
}

char *system::String::Data() const
{
	return *(char**)&buffer;
}

void system::String::Copy(const char *strText)
{
	char *src = const_cast<char*>(strText);
	char *dest = buffer;
	while (*src)
		*dest++ = *src++;
	*dest = 0;
}

/**
 * Dump the String data to serial console.
 * It's only meant fro testing/debugging.
 * Don't output to the framebuffer.
 */
void system::String::Debug()
{
	DEBUG_PTR("\n[system::String(", this);
	DEBUG_PTR(")] buffer = ", buffer);
	debug_size("; length = ", length);
	debug_print(" bytes.\nContents = ");
	debug_print(buffer);
	debug_print("\n");
}

void system::String::Append(const char *strText)
{
	/**
	 * Currently, does not check the bounds
	 * just don't exceed the length or limit.
	 */
	size_t len = k_strlen(buffer);
	char *p  = &buffer[len];
	char *p2 = const_cast<char*>(strText);
	while (*p2)
		*p++ = *p2++;

	*p = 0;
}

void system::String::Append(const char *title, void *paddr)
{
	Append(title);
	char buf[50];
	buf[0] = '0';
	buf[1] = 'x';
	char *p = &buf[2];
	uint64_t ud = (uint64_t)paddr;

	do
	{
		int remainder = ud % 16;
		*p++ = (remainder < 10) ? remainder + '0' : remainder + 'A' - 10;
	} while (ud /= 16);

	*p = 0;
	char *p1 = &buf[2];
	char *p2 = p - 1;
	while (p1 < p2)
	{
		char tmp = *p1;
		*p1 = *p2;
		*p2 = tmp;
		p1++;
		p2--;
	}
	Append(buf);
}

void system::String::AppendNumber(const char *title, size_t value)
{
	Append(title);
	char buf[50];
	char *p = &buf[0];
	size_t ud = value;

	do
	{
		int remainder = ud % 10;
		*p++ = remainder + '0';
	} while (ud /= 10);

	*p = 0;
	char *p1 = &buf[0];
	char *p2 = p - 1;
	while (p1 < p2)
	{
		char tmp = *p1;
		*p1 = *p2;
		*p2 = tmp;
		p1++;
		p2--;
	}
	Append(buf);
}


