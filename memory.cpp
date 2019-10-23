#include "system.h"

namespace system
{
	extern k_object *root;
	k_object *recycled = nullptr;
	uintptr_t next_available = 0;
	void ZeroMemory(uintptr_t addr, size_t len);
}

using namespace system;

static void SetTail(k_object *parent, k_object *obj)
{
	k_object* tmp = parent;
	while (tmp->next)
		tmp = tmp->next;
	tmp->next = obj;
}

void system::ZeroMemory(uintptr_t addr, size_t len)
{
	size_t pos = addr;
	size_t counter = len;
	while (counter)
	{
		if (counter < 8)
		{
			uint8_t* pchar = (uint8_t*)pos;
			for (size_t i = 0; i < counter; i++)
				*pchar++ = 0;
			return;
		}
		*((uintptr_t*)pos) = 0;
		pos += 8;
		counter -= 8;
	}
}

void *system::HeapAlloc(size_t len)
{
	if (recycled)
	{
		if (recycled->length >= len)
		{
			k_object *tmp = recycled;
			recycled = recycled->next;
			tmp->next = nullptr;
			tmp->length = len;

			// attach this to the end of root
			SetTail(root, tmp);
			ZeroMemory(tmp->data, len);
			DEBUG_PTR("\n[HeapAlloc] Returning ", tmp->data);
			DEBUG_PTR(" of object ", tmp);
			debug_print(" as new heap.\n");
			return reinterpret_cast<void*>(tmp->data);
		}

		k_object *prev = recycled;
		k_object *block = recycled->next;
		while (block)
		{
			if (block->length >= len)
			{
				prev->next = block->next;
				block->next = nullptr;
				block->length = len;
				SetTail(root, block);
				ZeroMemory(block->data, len);
				return reinterpret_cast<void*>(block->data);
			}
			block = block->next; // keep going
		}
	}

	debug_size("\n[HeapAlloc] Allocating new object as a container for ", len);
	debug_print(" bytes of memory.\n");
	// Not found, alloc new one
	// If this is the first time we gets called,
	// the next_available will be 0, initialize it.
	if (next_available == 0)
	{
		next_available = root->limit + 1;
		if (next_available & 0xfffffffffffff000)
		{
			next_available &= 0xfffffffffffff000;
			next_available += 0x1000;
		}
	}

	k_object *pNew = reinterpret_cast<k_object*>(next_available);
	pNew->length = len;
	pNew->data = next_available + sizeof(k_object);
	pNew->next = nullptr;

	next_available += len + sizeof(k_object);
	if (next_available & 0xfffffffffffff000)
	{
		next_available &= 0xfffffffffffff000;
		next_available += 0x1000;
	}
	pNew->limit = next_available - 1;

	SetTail(root, pNew);
	return reinterpret_cast<void*>(pNew->data);
}

static void RemoveNode(k_object *parent, k_object *pnode)
{
	k_object *prev = parent;
	k_object *tmp = parent->next;
	while (tmp)
	{
		if (tmp == pnode)
			break;
		prev = tmp;
		tmp = tmp->next;
	}
	prev->next = tmp->next;
	tmp->next = nullptr;
}

void system::HeapFree(void* pv)
{
	k_object *obj = reinterpret_cast<k_object*>(((uintptr_t)pv) - sizeof(k_object));
	if (obj->data != (uintptr_t)pv)
	{
		DEBUG_PTR("\n[HeapFree] Invalid pointer => ", pv);
		debug_print("\nPossibly not a heap, maybe static data or a stack.\n");
		return;
	}

	if (!recycled)
	{
		recycled = obj;
		
		// Find the node at root, and remove it
		RemoveNode(root, obj);
		DEBUG_PTR("\n[HeapFree] Set ", recycled);
		debug_print(" as first recycled object\n");
		return;
	}

	RemoveNode(root, obj);
	SetTail(recycled, obj);
}

void *operator new(size_t len)
{
	return HeapAlloc(len);
}

void *operator new[](size_t len)
{
	return HeapAlloc(len);
}

void operator delete(void *pv, size_t len) noexcept
{
	debug_size("\n[operator delete] length = ", len);
	debug_print("\n                  Delegate to HeapFree()\n");
	HeapFree(pv);
}

void operator delete[](void *pv, size_t len) noexcept
{
	debug_size("\noperator delete[] length = ", len);
	debug_print("\n                  Delegate to HeapFree()\n");
	HeapFree(pv);
}

void operator delete(void *pv) noexcept
{
	DEBUG_PTR("\n[operator delete(", pv);
	debug_print(") delegating to HeapFree()\n");
	HeapFree(pv);
}

void operator delete[](void *pv) noexcept
{
	DEBUG_PTR("\n[operator delete[] (", pv);
	debug_print(") delegating to HeapFree()\n");

	HeapFree(pv);
}


/**
 * CAVEAT is:
 * The Page Directory must already been setup in assembly code,
 * otherwise, we will have to do it in C (or in this case C++),
 * and that's very inconvenient, because we may have to convert
 * local tables virtual addresses into physical one (takes time).
 *
 * Currently we can still map to a valid address, unless
 * the virtual address given is not in 2 MB boundary.
 *
 * And if you try to map on other address outside PML4[511] and PDP[511]
 * based, the result is undefined.
 *
 * I should make it better soon.
 * This will be the next task to write a more clever page allocator,
 * and frame allocator, as a C++ class.
 *
 * :-)
 */

uintptr_t k_memory_map(uintptr_t phys, uintptr_t vaddr, size_t len)
{
	// fisrt, get PML4 table
	uintptr_t p4;
	asm volatile("mov %%cr3, %%rax" : "=a"(p4));

	// the get the pdp, index is started at bit 39
	uint64_t index = (vaddr >> 39) & 0x1ff;

	uint64_t *table = (uint64_t*)p4;
	if (table[index] & 0x1)
	{
		// actually it must be there, because we initialize it at bootstrap
		uintptr_t obj = table[index] & ~0xff;  // remove the trailing flags
		table = (uint64_t*)obj;                // the pdp
		index = (vaddr >> 30) & 0x1ff;         // index of page directory in the pdp
		if (table[index] & 0x1)
		{
			// the page directory is also must be there
			obj = table[index] & ~0xff;
			table = (uint64_t*)obj;           // the Page Directory
			index = (vaddr >> 22) & 0x1ff;    // this is the page frame (most likely is not there)

			///////////////////////////////////////////////////////////////////////
			// I guess we should remove this block, we don't use the index anyway.
			///////////////////////////////////////////////////////////////////////
			debug_size("\n[k_memory_map] Checking Page Directory index ", index);
			if (table[index] & 0x1)
			{
				// but if it is, then we should check if the address matches phys
				if ((table[index] & ~0xff) == phys)
				{
					debug_addr("\n[k_memory_map] already exists => ", table[index]);
					return vaddr;
				}


				// if not, just failed the mmap
				// because the address is currently in use
				debug_addr("\n[k_memory_map] the virtual addr at index is ", table[index] & ~0xff);
				return 0;
			}
			
			//////////////////////////////////////////////////////////////////////////
			// These are the real work, add new entries based on 0xFFFFFFFFC0000000,
			// which have a nice PML4[511], PDP[511], and PD[0].
			//
			// So we can start using the Page Directory on index 0, no matter what
			// the requested address is.
			//
			// TODO:
			// Simplify the following equations, it seems tedious, but at least
			// it is now correctly map the requested address, unlike before.
			//
			// We should have a more sophisticated way to calculate the index though.
			//
			// if it's not there, then create an entry.
			///////////////////////////////////////////////////////////////////////////
			size_t count = len / 0x200000;   // using 2 Mb pages
			if (len % 0x200000)
				count++;    // round up

			size_t last_bits = 0xffff; // 16 unused bits
			size_t start_p4  = (vaddr >> 39) & 0x1ff;
			size_t start_pdp = (vaddr >> 30) & 0x1ff;
			uintptr_t start_vaddr = (last_bits << 48) | (start_p4 << 39) | (start_pdp << 30);

			size_t distance  = vaddr - start_vaddr;  // in bytes
			size_t page_diff = distance / 0x200000;         // in pages (2 MB each)

			// in this case, because we use 0xffffffffc0000000 as the base
			// then page_diff is the new_index for 2 mb pages.
			uintptr_t start_addr = (phys - distance) & 0xfffffffffffff000;
			if (start_addr % 0x200000)
				start_addr -= (start_addr % 0x200000);


			debug_addr("\n[k_memory_map] We have a distance of ", distance);
			debug_size(" bytes\n[k_memory_map] between start addr and index ", page_diff);
			debug_addr(" in this Page Directory,\n[k_memory_map] where the start_addr is => ", start_addr);
			debug_size("\n[k_memory_map] Ignoring table index for 4K pages, and using index ", page_diff);

			//table[0] = start_addr | 0x83;

			size_t current_entry = page_diff;
			uintptr_t paddr = phys;
			if ((page_diff == 0) && (distance != 0))
			{
				// It means the requested address is something like:
				// 0xffffffffc010000000, where we still in index 0,
				// but there's a 0x100000 bytes distance in between
				// we cannot map phys to the 0xffffffffc0000000, it would be wrong.
				// since the 0xffffffffc0000000 should always belong to the real start address,
				// which is phys - distance (or, should we round 'distance' to 0x200000 page boundary?).
				//
				// We still got a page fault using this one.
				// TODO: Fix it soon.
				debug_addr("\n\033[1;31m[k_memory_map]\033[0m some distance between start page, but page diff == 0, start_addr will be ",
						paddr - (distance % 0x200000));

				paddr -= (distance % 0x200000);
				table[current_entry++] = paddr | 0x83;
				paddr += 0x200000;
				count--;
			}

			while (count)
			{
				table[current_entry++] = paddr | 0x83;
				paddr += 0x200000;
				count--;
			}

			debug_print("\n[k_memory_map] Flushing TLB.\n");
			asm volatile("mov %%cr3, %%rax" : "=a"(p4));
			asm volatile("mov %0, %%cr3" : : "a"(p4));
			debug_addr("[k_memory_map] Done, returning => ", vaddr);

			return vaddr;
		}
	}

	return 0;
}



