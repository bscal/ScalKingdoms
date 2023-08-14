#pragma once

#include "Core.h"

//#define AllocAlign(allocator, size, oldSize, align) \
//	allocator(nullptr, size, 0, align, file, func, line)
//#define ReallocAlign(allocator, ptr, newSize, oldSize, align) \
//	allocator(ptr, newSize, oldSize, align, file, func, line)
//#define FreeAlign(allocator, ptr, align) \
//	allocator(ptr, 0, 0, align, file, func, line)

//typedef void* (*Allocator)(void* ptr, size_t newSize, size_t oldSize, u32 align, const char* file, const char* func, u32 line);

void* FrameMalloc(size_t newSize, u32 align);
void* FrameRealloc(void* ptr, size_t newSize, size_t oldSize, u32 align);

#define AllocTempAlign(size, align) \
	FrameMalloc(size, align)
#define ReallocTempAlign(allocator, ptr, newSize, oldSize, align) \
	FrameRealloc(ptr, newSize, oldSize, align)

void InitMemory(size_t frameSize);
void FrameFree();

zpl_allocator GetFrameAllocator();

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
