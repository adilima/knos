#include "system.h"


using namespace system;

system::String::String()
{
	buffer = new char[256];
	length = 256;
}

system::String::String(size_t len)
{
	buffer = new char[len];
	length = len;
}

system::String::String(const char *src)
{
	Dup(src);
	// length was automaticaly set by Dup()
}

char *system::String::Dup(const char *src)
{
	// just in case...
	char *tmp = buffer;
	length = k_strlen(src);
	buffer = new char[length + 1];
	char *p = buffer;
	char *p2 = const_cast<char*>(src);
	while (*p2)
		*p++ = *p2++;
	*p = 0;
	return tmp;
}

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
		DEBUG_PTR("[sytem::String] deleting buffer => ", buffer);
		delete [] buffer;
		buffer = nullptr;
		length = 0;
	}
	DEBUG_PTR("\n[system::String(", this);
	debug_print(")] terminating.\n");
}

char *system::String::Data() const
{
	return *(char**)&buffer;
}

