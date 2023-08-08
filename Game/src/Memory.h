#pragma once

#include "Core.h"

#define SPersistent zpl_heap_allocator()
#define STemp state->FrameAllocator

#define SAlloc(allocator, size) zpl_alloc(allocator, size)
#define SFree(allocator, ptr) zpl_free(allocator, ptr)

#define SAllocTemp(size) SAlloc(GetGameState()->FrameAllocator, size)
#define SAllocHeap(size) SAlloc(zpl_heap_allocator(), size)

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
