#pragma once

#include "Core.h"

typedef zpl_allocator Allocator;

#define ALLOCATOR_HEAP zpl_heap_allocator()
#define ALLOCATOR_FRAME GetGameAllocator()

#define AllocAlign(allocator, size, align) \
	SAlloc(allocator, nullptr, size, 0, align, __FILE__, __FUNCTION__, __LINE__)
#define ReallocAlign(allocator, ptr, newSize, oldSize, align) \
	SAlloc(allocator, ptr, newSize, oldSize, align, __FILE__, __FUNCTION__, __LINE__)
#define FreeAlign(allocator, ptr, align) \
	SAlloc(allocator, ptr, 0, 0, align, __FILE__, __FUNCTION__, __LINE__)
#define AllocHeap(size, align) \
	AllocAlign(ALLOCATOR_HEAP, size, align)
#define ReallocHeap(ptr, newSize, oldSize, align) \
	ReallocAlign(ALLOCATOR_HEAP, ptr, newSize, oldSize, align)
#define FreeHeap(ptr, align) \
	FreeAlign(ALLOCATOR_HEAP, ptr, align)
#define AllocFrame(size, align) \
	AllocAlign(ALLOCATOR_FRAME, size, align)
#define ReallocFrame(ptr, newSize, oldSize, align) \
	ReallocAlign(ALLOCATOR_FRAME, ptr, newSize, oldSize, align)
#define FreeFrame(ptr, align) \
	FreeAlign(ALLOCATOR_FRAME, ptr, align)

_FORCE_INLINE_ void* 
SAlloc(Allocator allocator, void* ptr, size_t newSize, size_t oldSize, u32 align,
	const char* file, const char* func, u32 line)
{
	SASSERT(allocator.proc);
	void* res = nullptr;
	if (ptr)
	{
		if (newSize > 0)
			res = zpl_resize_align(allocator, ptr, oldSize, newSize, align);
		else
		{
			zpl_free(allocator, ptr);
			return res;
		}
	}
	else if (newSize > 0)
	{
		res = zpl_alloc_align(allocator, newSize, align);
	}
	SASSERT(res);
	return res;
}

_FORCE_INLINE_ bool
ValidateAllocator(Allocator allocator)
{
	return (allocator.proc);
}

#define SClear(ptr, size) zpl_memset(ptr, 0, size)
#define SCopy(dst, src, sz) zpl_memcopy(dst, src, sz)

typedef void* (*HashMapAlloc)(size_t, size_t);
typedef void(*HashMapFree)(void*, size_t);

inline void* DefaultAllocFunc(size_t sz, size_t oldSz)
{
	return zpl_alloc(zpl_heap_allocator(), sz);
}

inline void DefaultFreeFunc(void* ptr, size_t sz)
{
	return zpl_free(zpl_heap_allocator(), ptr);
}
