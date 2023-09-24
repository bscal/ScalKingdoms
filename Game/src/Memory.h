#pragma once

#include "Core.h"

#define TRACK_MEMORY 1

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

enum class Allocator : int
{
	Arena = 0,
	Frame,
	Malloc,
	MallocUntracked,

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

#define ZplAllocatorArena GetGameAllocator()
#define	ZplAllocatorFrame zpl_arena_allocator(&GetGameState()->Arena)
#define ZplAllocatorMalloc zpl_heap_allocator()

void*
GameAllocator_Internal(AllocatorAction allocatorType, void* ptr, size_t newSize, size_t oldSize, u32 align,
	const char* file, const char* func, u32 line);

void*
FrameAllocator_Internal(AllocatorAction allocatorType, void* ptr, size_t newSize, size_t oldSize, u32 align,
	const char* file, const char* func, u32 line);

void*
MallocAllocator_Internal(AllocatorAction allocatorType, void* ptr, size_t newSize, size_t oldSize, u32 align,
	const char* file, const char* func, u32 line);

void*
MallocUntrackedAllocator_Internal(AllocatorAction allocatorType, void* ptr, size_t newSize, size_t oldSize, u32 align,
	const char* file, const char* func, u32 line);

constexpr global AllocatorFunction Allocators[] =
{
	GameAllocator_Internal,
	FrameAllocator_Internal,
	MallocAllocator_Internal,
	MallocUntrackedAllocator_Internal
};
static_assert(ArrayLength(Allocators) == (int)Allocator::MaxAllocators,
	"AllocatorFunction array is not length of Allocators::MaxAllocators");

constexpr global const char* AllocatorNames[] =
{
	"Arena",
	"Frame",
	"Malloc",
	"MallocUntracked"
};
static_assert(ArrayLength(AllocatorNames) == (int)Allocator::MaxAllocators,
	"AllocatorFunction array is not length of Allocators::MaxAllocators");

inline bool
IsAllocatorValid(Allocator allocator)
{
	return ((int)allocator >= 0
		&& (int)allocator < (int)Allocator::MaxAllocators
		&& Allocators[(int)allocator]);
}

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
