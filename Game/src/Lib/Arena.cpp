#include "Arena.h"

#include "Memory.h"
#include "Utils.h"

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
	size_t res = arena->Size - (arena->TotalAllocated + ArenaAlignment(arena, alignment));
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
	snapshot.Arena->TotalAllocated--;
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