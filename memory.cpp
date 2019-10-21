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
		DEBUG_PTR("\n[HeapFree] Set ", pv);
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
	debug_print("\nDelegate to HeapFree()\n");
	HeapFree(pv);
}

void operator delete[](void *pv, size_t len) noexcept
{
	debug_size("\noperator delete[] length = ", len);
	debug_print("\nDelegate to HeapFree()\n");
	HeapFree(pv);
}

void operator delete(void *pv) noexcept
{
	HeapFree(pv);
}

void operator delete[](void *pv) noexcept
{
	HeapFree(pv);
}




