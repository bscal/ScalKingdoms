#pragma once

#include "Base.h"

#define TRACK_MEMORY 1

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

#ifndef StackAlloc
#if defined(__clang__) || defined(__GNUC__)
#include <alloca.h>
#define StackAlloc(size) alloca(size)
#elif defined(_MSC_VER)
#include <malloc.h>
#define StackAlloc(size) _malloca(size)
#else
#define StackAlloc(size) static_assert(false, "StackAlloc is not supported")
#endif
#endif

SAllocatorProc(GameAllocatorProc);

SAllocatorProc(FrameAllocatorProc);

SAllocatorProc(MallocAllocatorProc);

SAllocatorProc(ArenaAllocatorProc);

#define SAllocatorGeneral() (SAllocator{ GameAllocatorProc, nullptr })
#define SAllocatorFrame() (SAllocator{ FrameAllocatorProc, nullptr })
#define SAllocatorMalloc() (SAllocator{ MallocAllocatorProc, nullptr })
#define SAllocatorArena(arena) (SAllocator{ ArenaAllocatorProc, arena })

#define SAlloc(allocator, size)	\
	allocator.Proc(ALLOCATOR_TYPE_MALLOC, allocator.Data, nullptr, size, 0, DEFAULT_ALIGNMENT, __FILE__, __FUNCTION__, __LINE__)

#define SAllocAlign(allocator, size, align) \
	allocator.Proc(ALLOCATOR_TYPE_MALLOC, allocator.Data, nullptr, size, 0, align, __FILE__, __FUNCTION__, __LINE__)

#define SCalloc(allocator, size) \
	SZero(SAlloc(allocator, size), size)

#define SRealloc(allocator, ptr, size, oldSize)	\
	allocator.Proc(ALLOCATOR_TYPE_REALLOC, allocator.Data, ptr, size, oldSize, DEFAULT_ALIGNMENT, __FILE__, __FUNCTION__, __LINE__)

#define SFree(allocator, ptr)	\
	allocator.Proc(ALLOCATOR_TYPE_FREE, allocator.Data, ptr, 0, 0, DEFAULT_ALIGNMENT, __FILE__, __FUNCTION__, __LINE__)

#define SCopy(dst, src, sz) memcpy(dst, src, sz)
#define SMemMove(dst, src, sz) memmove(dst, src, sz)
#define SZero(ptr, sz) memset(ptr, 0, sz)

void InitializeMemoryTracking();
void ShutdownMemoryTracking();

void Internal_PushMemoryIgnoreFree();
void Internal_PopMemoryIgnoreFree();
void Internal_PushMemoryAdditionalInfo(const char* file, const char* func, int line);
void Internal_PopMemoryAdditionalInfo();
void Internal_PushMemoryPointer(void* address);
void Internal_PopMemoryPointer();

#if TRACK_MEMORY
#define PushMemoryIgnoreFree() Internal_PushMemoryIgnoreFree()
#define PopMemoryIgnoreFree() Internal_PopMemoryIgnoreFree()
#define PushMemoryAdditionalInfo(file, func, line) Internal_PushMemoryAdditionalInfo(file, func, line)
#define PopMemoryAdditionalInfo() Internal_PopMemoryAdditionalInfo()
#define PushMemoryPointer(address) Internal_PushMemoryPointer(address)
#define PopMemoryPointer() Internal_PopMemoryPointer();
#else
#define PushMemoryIgnoreFree() 
#define PopMemoryIgnoreFree() 
#define PushMemoryAdditionalInfo(file, func, line)
#define PopMemoryAdditionalInfo()
#define PushMemoryPointer(address)
#define PopMemoryPointer()
#endif

