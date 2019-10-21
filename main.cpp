#include "system.h"
#include "multiboot2.h"


void debug_addr(const char* title, uintptr_t addr)
{
	size_t ud = (size_t)addr;
	char buf[50];
	char *p = buf;
	debug_print(title);
	debug_print("0x");

	do
	{
		int remainder = ud % 16;
		*p++ = (remainder < 10) ? remainder + '0' : remainder + 'A' - 10;
	} while (ud /= 16);

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
	debug_print(buf);
}

void debug_size(const char* title, size_t nsize, bool bAppendBytes)
{
	size_t ud = nsize;
	char buf[50];
	char *p = buf;
	debug_print(title);

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
	debug_print(buf);
	if (bAppendBytes)
		debug_print(" bytes");
}

uint64_t read_tsc(void)
{
	uint64_t retval;
	asm volatile(".byte 0x10, 0x31" : "=A"(retval));
	return retval;
}

namespace system { k_object *root = nullptr; }

extern "C"
void kmain(uintptr_t mbi)
{
	debug_init();
	debug_print("\n\033[1;31m------------ C++ Started -------------\033[0m\n");

	// Testing local variable instance addresses
	// Most likely it's using lower half address.
	int intLocalVar = 1234;

	static int sValue = 1;
	DEBUG_PTR("\n[kmain] Local Integer Variable address (stack) => ", &intLocalVar);
	DEBUG_PTR("\n[constant] Testing constant addr => ", "This is a test");
	DEBUG_PTR("\n[static] Testing static local var => ", &sValue);
	
	debug_addr("\n[kmain] MBI => ", mbi);
	debug_size("; size = ", *((unsigned*)mbi), true);

	multiboot_tag* tag = reinterpret_cast<multiboot_tag*>(mbi + 8);
	uintptr_t pos = mbi + 8;

	uintptr_t vga = 0xb8000;

	debug_size("\n[kmain] MBI First tag -> ", tag->type);

	while (tag->type != MULTIBOOT_TAG_TYPE_END)
	{
		if (tag->type == MULTIBOOT_TAG_TYPE_FRAMEBUFFER)
		{
			multiboot_tag_framebuffer *fbtag = reinterpret_cast<multiboot_tag_framebuffer*>(tag);
			debug_addr("\n[kmain] Framebuffer found at ", fbtag->common.framebuffer_addr);
			debug_size("\n                    width   : ", fbtag->common.framebuffer_width);
			debug_size("\n                    height  : ", fbtag->common.framebuffer_height);
			debug_size("\n                    bpp     : ", fbtag->common.framebuffer_bpp);
			debug_size("\n                    pitch   : ", fbtag->common.framebuffer_pitch);
			if (fbtag->common.framebuffer_type == 2)
			{
				debug_print("\nDummy VGA/Text 80x25 framebuffer console.\nClearing the screen (using turquoise color).\n");

				char *ptr = (char*)(fbtag->common.framebuffer_addr);
				for (int i = 0; i < (80 * 25); i++)
				{
					ptr[0] = 0x20;
					ptr[1] = 0x3f;
					ptr += 2;
				}
				vga = 0xffffffff80000000 + fbtag->common.framebuffer_addr;
			}
			else
			{
				if (fbtag->common.framebuffer_addr == 0xFD000000)
					debug_print("\nQEMU VBE based framebuffer.\n");
				else if (fbtag->common.framebuffer_addr == 0x80000000)
					debug_print("\nQEMU UEFI based framebuffer.\n");
				else
					debug_print("\nMost likely we are on REAL hardware, and this text may not be displayed.\n");
			}
		}

		pos = (pos + tag->size + 7) & ~7;
		tag = reinterpret_cast<multiboot_tag*>(pos);
	}

	uintptr_t heap_start = (uintptr_t)&kernel_virt_end;
	system::k_object *pHeap = reinterpret_cast<system::k_object*>(heap_start);
	pHeap->length = 4096;
	pHeap->data = heap_start + sizeof(system::k_object);
	pHeap->next = nullptr;
	pHeap->limit = pHeap->data + 4096;
	if (pHeap->limit & 0xFFFFFFFFFFFFF000)
	{
		pHeap->limit &= 0xFFFFFFFFFFFFF000;
		pHeap->limit += 0x1000;
	}

	system::root = pHeap;
	DEBUG_PTR("\n[kmain] Heap start => ", heap_start);
	DEBUG_PTR("\n[kmain] data addr    => ", &pHeap->data);
	DEBUG_PTR("\n             data    => ", pHeap->data);
	DEBUG_PTR("\n             limit   => ", pHeap->limit);

	debug_print("\n[kmain] Testing newly written HeapAlloc(), Free(), and C++ operators\n");
	int* pInt = new int[25];
	DEBUG_PTR("[kmain] Created array of int[25] starting from => ", pInt);
	debug_size("; value[0] = ", pInt[0]);
	debug_print("\nDelete the pointer.\n");
	delete [] pInt;

	system::String obj("[String] This is a test");

	obj.Show(vga);
	debug_print("\n[system::String]::Data() => ");
	debug_print(obj.Data());

	system::String* pStr = new system::String("[kmain] This is system::String from the heap.");
	pStr->Show(vga + 160);

	debug_print("\033[1;32m\n[kmain] system::String->Data() = ");
	debug_print(pStr->Data());
	debug_print("\033[0m\n");
	delete pStr;

	debug_size("\n[kmain] testing k_strlen() with \'This is a test\' => ",
			k_strlen("This is a test"));

	debug_print("\n[kmain] test completed.\nSleeping forever... bye...\n\n");

	while (1)
	{
		asm volatile("nop");
	}
}

