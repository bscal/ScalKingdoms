#pragma once

#include "Base.h"
#include "Memory.h"

struct Arena
{
	void* Memory;
	size_t Size;
	size_t TotalAllocated;
	int TempCount;
	SAllocator Allocator;
};

//! Initialize memory arena from existing memory region.
inline void ArenaCreate(Arena* arena, void* start, size_t size);

//! Initialize memory arena using existing memory SAllocator.
inline void ArenaCreateFromAllocator(Arena* arena, SAllocator backing, size_t size);

//! Initialize memory arena within an existing parent memory arena.
inline void ArenaCreateFromArena(Arena* arena, Arena* parent_arena, size_t size);

//! Release the memory used by memory arena.
inline void ArenaFree(Arena* arena);

//! Retrieve memory arena's aligned allocation address.
inline size_t ArenaAlignment(Arena* arena, size_t alignment);

//! Retrieve memory arena's remaining size.
inline size_t ArenaSizeRemaining(Arena* arena, size_t alignment);

struct ArenaSnapshot
{
	Arena* Arena;
	size_t OriginalTotalAllocated;
};

//! Capture a snapshot of used memory in a memory arena.
inline ArenaSnapshot ArenaSnapshotBegin(Arena* arena);

//! Reset memory arena's usage by a captured snapshot.
inline void ArenaSnapshotEnd(ArenaSnapshot snapshot);

inline void* ArenaPush(Arena* arena, size_t size);
inline void* ArenaPushZero(Arena* arena, size_t size);

#define ArenaPushArray(arena, type, count) (type*)ArenaPush((arena), sizeof(type)*(count))
#define ArenaPushArrayZero(arena, type, count) (type*)ArenaPushZero((arena), sizeof(type)*(count))
#define ArenaPushStruct(arena, type) ArenaPushArray(arena, type, 1)
#define ArenaPushStructZero(arena, type) ArenaPushArrayZero(arena, type, 1)

inline void ArenaPop(Arena* arena, size_t size);

// *************

//! Initialize memory arena from existing memory region.
void ArenaCreate(Arena* arena, void* start, size_t size)
{
	arena->Allocator = {};
	arena->Memory = start;
	arena->Size = size;
	arena->TotalAllocated = 0;
	arena->TempCount = 0;
}

//! Initialize memory arena using existing memory SAllocator.
void ArenaCreateFromAllocator(Arena* arena, SAllocator backing, size_t size)
{
	arena->Allocator = backing;
	arena->Memory = SAlloc(backing, size);
	arena->Size = size;
	arena->TotalAllocated = 0;
	arena->TempCount = 0;
}

//! Initialize memory arena within an existing parent memory arena.
void ArenaCreateFromArena(Arena* arena, Arena* parentArena, size_t size)
{
	ArenaCreateFromAllocator(arena, SAllocatorArena(parentArena), size);
}

//! Release the memory used by memory arena.
void ArenaFree(Arena* arena)
{
	if (IsAllocatorValid(arena->Allocator))
	{
		SFree(arena->Allocator, arena->Memory);
		arena->Memory = nullptr;
	}
}

//! Retrieve memory arena's aligned allocation address.
size_t ArenaAlignment(Arena* arena, size_t alignment)
{
	SAssert(IsPowerOf2(alignment));
	size_t alignmentOffset = 0;
	size_t resultPointer = (size_t)arena->Memory + arena->TotalAllocated;
	size_t mask = alignment - 1;
	if (resultPointer & mask)
		alignmentOffset = alignment - (resultPointer & mask);

	return alignmentOffset;
}

//! Retrieve memory arena's remaining size.
size_t ArenaSizeRemaining(Arena* arena, size_t alignment)
{
	size_t res = arena->Size - arena->TotalAllocated;
	return res;
}

//! Capture a snapshot of used memory in a memory arena.
ArenaSnapshot ArenaSnapshotBegin(Arena* arena)
{
	ArenaSnapshot snapshot;
	snapshot.Arena = arena;
	snapshot.OriginalTotalAllocated = arena->TotalAllocated;
	arena->TempCount++;
	return snapshot;
}

//! Reset memory arena's usage by a captured snapshot.
void ArenaSnapshotEnd(ArenaSnapshot snapshot)
{
	SAssert(snapshot.Arena->TotalAllocated >= snapshot.OriginalTotalAllocated);
	SAssert(snapshot.Arena->TempCount > 0);
	snapshot.Arena->TotalAllocated = snapshot.OriginalTotalAllocated;
	snapshot.Arena->TempCount--;
}

void* ArenaPush(Arena* arena, size_t size)
{
	void* res;
	size_t totalSize = AlignSize(size, 16);

	if (arena->TotalAllocated + totalSize > arena->Size)
	{
		SError("Arena out of memory");
		return nullptr;
	}

	res = (void*)((size_t)arena->Memory + arena->TotalAllocated);
	arena->TotalAllocated += totalSize;
	SAssert(res);
	return res;
}

void* ArenaPushZero(Arena* arena, size_t size)
{
	void* res = ArenaPush(arena, size);
	SZero(res, AlignSize(size, 16));
	return res;
}

void ArenaPop(Arena* arena, size_t size)
{
	arena->TotalAllocated -= AlignSize(size, 16);
}
