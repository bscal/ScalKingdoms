#pragma once

#include "Core.h"

enum class Allocator : int
{
	Arena = 0,
	Frame,
	Malloc,

	MaxAllocators
};

enum AllocatorAction
{
	ALLOCATOR_TYPE_MALLOC = 0,
	ALLOCATOR_TYPE_REALLOC,
	ALLOCATOR_TYPE_FREE,

	ALLOCATOR_TYPE_COUNT
};

typedef void* (*AllocatorFunction)(AllocatorAction allocatorType, void* ptr, size_t newSize, size_t oldSize, u32 align,
	const char* file, const char* func, u32 line);

extern zpl_allocator GetGameAllocator();
extern zpl_allocator GetFrameAllocator();

#define ZplAllocatorArena GetGameAllocator()
#define	ZplAllocatorFrame GetFrameAllocator()
#define ZplAllocatorMalloc zpl_heap_allocator()

inline bool
IsAllocatorValid(Allocator allocator)
{
	return ((int)allocator >= 0 && (int)allocator < (int)Allocator::MaxAllocators);
}

inline void*
GameAllocator_Internal(AllocatorAction allocatorType, void* ptr, size_t newSize, size_t oldSize, u32 align,
	const char* file, const char* func, u32 line)
{
	void* res;

	switch (allocatorType)
	{
	case (ALLOCATOR_TYPE_MALLOC):
	{
		res = zpl_alloc_align(GetGameAllocator(), newSize, align);
	} break;

	case (ALLOCATOR_TYPE_REALLOC):
	{
		res = zpl_resize_align(GetGameAllocator(), ptr, oldSize, newSize, align);
	} break;

	case (ALLOCATOR_TYPE_FREE):
	{
		zpl_free(GetGameAllocator(), ptr);
		res = nullptr;
	} break;

	default:
		SError("Invalid allocator type");
		res = nullptr;
	}

#if SCAL_DEBUG
	if (allocatorType != ALLOCATOR_TYPE_FREE)
		SAssert( res);
#endif

	return res;
}

inline void*
FrameAllocator_Internal(AllocatorAction allocatorType, void* ptr, size_t newSize, size_t oldSize, u32 align,
	const char* file, const char* func, u32 line)
{
	void* res;

	switch (allocatorType)
	{
	case (ALLOCATOR_TYPE_MALLOC):
	{
		res = zpl_alloc_align(GetFrameAllocator(), newSize, align);
	} break;

	case (ALLOCATOR_TYPE_REALLOC):
	{
		res = zpl_resize_align(GetFrameAllocator(), ptr, oldSize, newSize, align);
	} break;

	case (ALLOCATOR_TYPE_FREE):
	{
		// Frame allocator frees at end of frame
		res = nullptr;
	} break;

	default:
		SError("Invalid allocator type");
		res = nullptr;
	}

#if SCAL_DEBUG
	if (allocatorType != ALLOCATOR_TYPE_FREE)
		SAssert(res);
#endif

	return res;
}

inline void*
MallocAllocator_Internal(AllocatorAction allocatorType, void* ptr, size_t newSize, size_t oldSize, u32 align,
	const char* file, const char* func, u32 line)
{
	void* res;

	switch (allocatorType)
	{
	case (ALLOCATOR_TYPE_MALLOC):
	{
		res = zpl_alloc_align(zpl_heap_allocator(), newSize, align);
	} break;

	case (ALLOCATOR_TYPE_REALLOC):
	{
		res = zpl_resize_align(zpl_heap_allocator(), ptr, oldSize, newSize, align);
	} break;

	case (ALLOCATOR_TYPE_FREE):
	{
		zpl_free(zpl_heap_allocator(), ptr);
		res = nullptr;
	} break;

	default:
		SError("Invalid allocator type");
		res = nullptr;
	}

#if SCAL_DEBUG
	if (allocatorType != ALLOCATOR_TYPE_FREE)
		SAssert(res);
#endif

	return res;
}

constexpr global_var AllocatorFunction Allocators[] =
{
	GameAllocator_Internal,
	FrameAllocator_Internal,
	MallocAllocator_Internal
};
static_assert(ArrayLength(Allocators) == (int)Allocator::MaxAllocators,
	"AllocatorFunction array is not length of Allocators::MaxAllocators");

#if SCAL_DEBUG
inline AllocatorFunction DebugIndexAllocator(int allocatorIdx)
{
	SAssert(allocatorIdx >= 0 && allocatorIdx < (int)Allocator::MaxAllocators);
	return Allocators[allocatorIdx];
}
#define IndexAllocators(allocatorIdx) DebugIndexAllocator(allocatorIdx)
#else
#define IndexAllocators(allocatorIdx) Allocators[allocatorIdx]
#endif

#define SMalloc(allocator, size) \
	IndexAllocators((int)allocator)(ALLOCATOR_TYPE_MALLOC, nullptr, size, 0, 16, __FILE__, __FUNCTION__, __LINE__)

#define SRealloc(allocator, ptr, size, oldSize) \
	IndexAllocators((int)allocator)(ALLOCATOR_TYPE_REALLOC, ptr, size, oldSize, 16, __FILE__, __FUNCTION__, __LINE__)

#define SFree(allocator, ptr) \
	IndexAllocators((int)allocator)(ALLOCATOR_TYPE_FREE, ptr, 0, 0, 16, __FILE__, __FUNCTION__, __LINE__)

#define SClear(ptr, size) zpl_memset(ptr, 0, size)
#define SCopy(dst, src, sz) zpl_memcopy(dst, src, sz)
