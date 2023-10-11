#pragma once

#include "Core.h"

enum class SAllocatorId : int
{
	General = 0,
	Frame,
	Malloc,
	Arena,

	Max
};

enum SAllocatorType : int
{
	ALLOCATOR_TYPE_MALLOC,
	ALLOCATOR_TYPE_REALLOC,
	ALLOCATOR_TYPE_FREE
};

#define SAllocatorProc(name)\
	void* name(SAllocatorType allocatorType, void* data, void* ptr, \
	size_t newSize, size_t oldSize, u32 align, const char* file, \
	const char* func, u32 line)
typedef SAllocatorProc(AllocatorProc);

struct SAllocator
{
	AllocatorProc* Proc;
	void* Data;
};

inline bool
IsAllocatorValid(SAllocator allocator)
{
	return allocator.Proc;
}