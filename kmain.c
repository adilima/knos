/**
 * Use this file as an alternative to main.cpp,
 * if you does not like C++
 * 
 * I am going to upload some C style starter codes soon.
 */

typedef unsigned char  uchar;
typedef unsigned short ushort;
typedef unsigned long  ulong;
typedef unsigned int   uint32;
typedef unsigned long  uint64;
typedef unsigned long  k_addr_t;

#ifndef size_t
typedef unsigned long  size_t;
#endif

static char* print(char* pos, const char* str)
{
	char* p = (char*)str;
	while (*p)
	{
		*pos++ = *p++;
		*pos++ = 0x1f;
	}
	return pos;
}

static void clear(char* pos)
{
	for (int i = 0; i < (160*25); i++)
	{
		*pos++ = ' ';
		*pos++ = 0x1f;
	}
}

/**
 * If nothing works, then we may need to implement
 * our own serial access here.
 */
static inline uchar k_debug_read(ushort index)
{
	uchar retval;
	asm volatile ("inb (%%dx), %%al" : "=a"(retval) : "d"(0x3f8+index));
	return retval;
}

static inline void k_debug_write(ushort index, uchar value)
{
	asm volatile ("outb %%al, (%%dx)" : : "a"(value), "d"(0x3f8+index));
}

static void k_debug_init(void)
{
	k_debug_write(1, 0);
	k_debug_write(3, 128);
	k_debug_write(0, 3);
	k_debug_write(1, 0);
	k_debug_write(3, 3);
	k_debug_write(2, 199);
	k_debug_write(4, 11);
}

static void k_debug_print(const char* str)
{
	char* c = (char*)str;
	uchar status = k_debug_read(5) & 0x20;
	while (!status)
		status = k_debug_read(5) & 0x20;
	while (*c)
		k_debug_write(0, *c++);
	k_debug_write(0, '\n');
}

static void k_debug_addr(const char* title, k_addr_t addr)
{
	char buf[50];
	char* p = (char*)title;
	ulong ud = addr;
	
	uchar status = k_debug_read(5) & 0x20;
	while (!status)
		status = k_debug_read(5);
	while (*p)
		k_debug_write(0, *p++);

	buf[0] = '0';
	buf[1] = 'x';
	p = &buf[2];
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

	status = k_debug_read(5) & 0x20;
	while (!status)
		status = k_debug_read(5) & 0x20;

	p = buf;
	while (*p)
		k_debug_write(0, *p++);

	/* add new line */
	k_debug_write(0, '\n');
}

static void k_debug_size(const char* title, size_t nsize)
{
	char buf[50];
	char* p = (char*)title;
	ulong ud = nsize;
	
	uchar status = k_debug_read(5) & 0x20;
	while (!status)
		status = k_debug_read(5);
	while (*p)
		k_debug_write(0, *p++);

	p = buf;
	do
	{
		int remainder = ud % 10;
		*p++ = remainder + '0';
	} while (ud /= 10);
	
	*p = 0;
	char *p1 = buf;
	char *p2 = p - 1;

	while (p1 < p2)
	{
		char tmp = *p1;
		*p1 = *p2;
		*p2 = tmp;
		p1++;
		p2--;
	}

	status = k_debug_read(5) & 0x20;
	while (!status)
		status = k_debug_read(5) & 0x20;

	p = buf;
	while (*p)
		k_debug_write(0, *p++);

	/* add new line */
	k_debug_write(0, '\n');
}


#define DEBUG_PTR(title, val) 	k_debug_addr(((const char*)(title)), ((k_addr_t)(val)))

void kmain(unsigned long mbi)
{

	unsigned long pos = 0xffffffff800b8000;
	clear((char*)pos);
	print((char*)pos, "=============== C Kernel Started ==============");
	pos += 160;
	print((char*)pos, "[kmain] Bootstrap is more stable now.");
	
	k_debug_init();
	k_debug_print("\n\033[1;31m============================= C Kernel Started ===========================");
	k_debug_print("If we got here, then the mystery of Virtual Address space has been solved.\n:-)\n\033[1;34m");
	k_debug_addr("[kmain] Got MBI address: ", mbi);

	unsigned* ptr = (unsigned*)mbi;
	k_debug_size("\nMBI Size = ", ptr[0]);

	ptr = (unsigned*)(mbi + 8);
	DEBUG_PTR("First tag => ", ptr[0]);


	k_debug_print("[kmain] Ended (for now)...\033[0m\n");
	while (1);
}

