#include "Memory.h"

zpl_stack_memory FrameStack;
zpl_allocator FrameAllocator;
bool IsInitialized;

void InitMemory(size_t frameSize)
{
	IsInitialized = true;

	zpl_stack_memory_init(&FrameStack, zpl_heap_allocator(), frameSize);
	FrameAllocator = zpl_stack_allocator(&FrameStack);
}

void* FrameMalloc(size_t newSize, u32 align)
{
	SASSERT(IsInitialized);
	if (IsInitialized)
	{
		return zpl_alloc_align(FrameAllocator, newSize, align);
	}
	return nullptr;
}

void* FrameRealloc(void* ptr, size_t newSize, size_t oldSize, u32 align)
{
	SASSERT(IsInitialized);
	if (IsInitialized)
	{
		return zpl_resize_align(FrameAllocator, ptr, oldSize, newSize, align);
	}
	return nullptr;
}

void FrameFree()
{
	SASSERT(IsInitialized);
	if (IsInitialized)
	{
		zpl_free_all(FrameAllocator);
	}
}

zpl_allocator GetFrameAllocator()
{
	return FrameAllocator;
}
