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

namespace system { 
	k_object *root = nullptr;
}


extern "C"
void kmain(uintptr_t mbi)
{
	debug_init();
	debug_print("\n\033[1;31m------------------- C++ Started ------------------\033[0m\n");

	// Testing local variable instance addresses
	// Most likely it's using lower half address.
	int intLocalVar = 12345678;
	static int sValue = 1;

	DEBUG_PTR("\n[kmain]    Local Integer Variable address (stack) => ", &intLocalVar);
	DEBUG_PTR("\n[constant] Testing constant addr => ", "This is a test");
	DEBUG_PTR("\n[static]   Testing static local var => ", &sValue);
	
	debug_addr("\n[kmain] MBI => ", mbi);
	debug_size("; size = ", *((unsigned*)mbi), true);

	multiboot_tag* tag = reinterpret_cast<multiboot_tag*>(mbi + 8);
	multiboot_tag_module *module_tag = nullptr;

	uintptr_t pos = mbi + 8;

	uintptr_t vga = 0xb8000;

	multiboot_tag_framebuffer *fbtag = nullptr;

	debug_size("\n[kmain] MBI First tag -> ", tag->type);

	while (tag->type != MULTIBOOT_TAG_TYPE_END)
	{
		if (tag->type == MULTIBOOT_TAG_TYPE_FRAMEBUFFER)
		{
			fbtag = reinterpret_cast<multiboot_tag_framebuffer*>(tag);
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

				debug_print("[kmain] Framebuffer object will be created after we initialize the heap.\n");
			}
		}
		else if (tag->type == MULTIBOOT_TAG_TYPE_MODULE)
		{
			module_tag = reinterpret_cast<multiboot_tag_module*>(tag);
			DEBUG_PTR("\n[kmain] Found a module at => ", module_tag);
			debug_print("; cmdline = ");
			debug_print(module_tag->cmdline);

			if (k_str_equal(module_tag->cmdline, "font", k_strlen("font")))
				debug_print("\n[kmain] The module is a font.");

			debug_print("\n");
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

	debug_size("\n[kmain] testing k_strlen() with \'This is a test\' => ",
			k_strlen("This is a test"));
	debug_print("\n\033[1;31m[kmain] Heap initialized.\n\033[0m");

	system::String *pStr = new system::String();
	pStr->Copy("\033[1;33m[kmain] system::String using heap.\n\033[0m");
	pStr->Debug();

	delete pStr;
	pStr = new system::String("\033[1;34m[kmain] system::String using heap, and designated initializer.\n\033[0m");
	pStr->Debug();
	delete pStr;

	debug_print("[kmain] Testing framebuffer\n");
	system::fbdev = new system::framebuffer((uintptr_t)fbtag);


	if (system::fbdev->virt)
	{
		debug_print("\n[kmain] Looks like the framebuffer is ready, clearing screen.\n");
		system::fbdev->clear();
		if (module_tag)
		{
			system::fbdev->font = reinterpret_cast<PSF_FONT*>(module_tag->mod_start);
			system::fbdev->show_test();
		}
	}

	system::box *pBox = new system::box();
	pBox->set_title("Terminal");
	pBox->show(10, 50);
	
	system::box *pRTCBox = new system::box();
	pRTCBox->set_title("RTC Time");
	pRTCBox->show(25, 200);

	debug_print("\n[kmain] Time to test the keyboard inputs.\n");
	
	char lastChar = system::getchar();

	char arr[80];
	
	while (lastChar)
	{	
		// display current time using new RTC function
		while (k_rtc_update_in_progress());
		
		uint8_t hours = k_get_rtc(RTC_HOURS);
		uint8_t minutes = k_get_rtc(RTC_MINUTES);
		uint8_t secs = k_get_rtc(RTC_SECONDS);
		
		////////////////////////////////////////////////
		// actually we must convert these values
		// otherwise it will be displayed incorrectly
		// if these are in BCD.
		//
		// Or we can choose to display BCD values as 'HEX',
		// but of course without '0x' prefixes.
		////////////////////////////////////////////////
		uint8_t regb = k_rtc_get_value(0x0b);
		if ((regb & 0x04) == 0)
		{
		    //////////////////////////////////////////////////////////////////////////////
		    // This means the values are in BCD format.
		    // If the time is 22:16:34, then it can be
		    // displayed 'correctly' as:
		    // 0x22:0x16:0x34
		    // Where 0x22 is actually 34, 0x16 is 22, and 0x34 surely is 52
		    //
		    // Confused?
		    // :-)
		    /////////////////////////////////////////////////////////////////////////////
		    hours   = ((hours & 0xf) + (((hours & 0x70) / 16) * 10)) | (hours & 0x80);
		    minutes = (minutes & 0xf) + ((minutes / 16) * 10);
		    secs    = (secs & 0xf) + ((secs / 16) * 10);
		}
		
		arr[0] = 0;
		
		char *pos = k_strcat_l(arr, "Current Time: ", hours);
		pos = k_strcat_l(pos, ":", minutes);
		pos = k_strcat_l(pos, ":", secs);
		pRTCBox->set_text(arr);
		pRTCBox->show(50, 250);
		
		int nIndex = 0;
		if (lastChar == -1)
		{
			lastChar = system::getchar();
			continue;
		}

    	arr[nIndex] = lastChar;
		arr[nIndex+1] = 0;
		nIndex++;
		if (nIndex >= 80)
		{
		    pBox->set_text("cmd> ");
		    lastChar = system::getchar();
		    pBox->show(10, 50);
		    continue;
		}
		pBox->set_text(arr);
		lastChar = system::getchar();
		pBox->show(10, 50);
	}
}


char *k_strcat(char *p1, const char *p2)
{
    char *dest = p1;
    char *src = const_cast<char*>(p2);
    while (*src)
        *dest++ = *src++;
    *dest = 0;
    return dest;
}

char *k_strcat_x(char *p1, const char *title, uintptr_t value)
{
    char *last = k_strcat(p1, title);
    *last++ = '0';
    *last++ = 'x';
    
    size_t ud = value;
    
    char *p = last;
    
    do
    {
        int remainder = ud % 16;
        *last++ = (remainder < 10) ? remainder + '0' : remainder + 'A' - 10;
    } while (ud /= 16);
    
    *last = 0;
    char *p2 = last - 1;
    while (p < p2)
    {
        char tmp = *p;
        *p = *p2;
        *p2 = tmp;
        p++;
        p2--;
    }
    return last;
}

char *k_strcat_l(char *p1, const char *title, size_t value)
{
    char *last = k_strcat(p1, title);
    size_t ud = value;
    
    char *p = last;
    
    do
    {
        int remainder = ud % 10;
        *last++ = remainder + '0';
    } while (ud /= 10);
    
    *last = 0;
    char *p2 = last - 1;
    while (p < p2)
    {
        char tmp = *p;
        *p = *p2;
        *p2 = tmp;
        p++;
        p2--;
    }
    return last;
}

