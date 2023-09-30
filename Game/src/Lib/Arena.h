#pragma once

#include "Allocator.h"

struct MemArena
{
	void* Memory;
	size_t Size;
	size_t TotalAllocated;
	int TempCount;
	SAllocator Allocator;
};

//! Initialize memory arena from existing memory region.
void ArenaCreate(MemArena* arena, void* start, size_t size);

//! Initialize memory arena using existing memory SAllocator.
void ArenaCreateFromAllocator(MemArena* arena, SAllocator backing, size_t size);

//! Initialize memory arena within an existing parent memory arena.
void ArenaCreateFromArena(MemArena* arena, MemArena* parent_arena, size_t size);

//! Release the memory used by memory arena.
void ArenaFree(MemArena* arena);

//! Retrieve memory arena's aligned allocation address.
size_t ArenaAlignment(MemArena* arena, size_t alignment);

//! Retrieve memory arena's remaining size.
size_t ArenaSizeRemaining(MemArena* arena, size_t alignment);


struct ArenaSnapshot
{
	MemArena* Arena;
	size_t OriginalTotalAllocated;
};

//! Capture a snapshot of used memory in a memory arena.
ArenaSnapshot ArenaSnapshotBegin(MemArena* arena);

//! Reset memory arena's usage by a captured snapshot.
void ArenaSnapshotEnd(ArenaSnapshot snapshot);

void* ArenaPush(MemArena* arena, size_t size);
void* ArenaPushZero(MemArena* arena, size_t size);

#define ArenaPushArray(arena, type, count) (type*)ArenaPush((arena), sizeof(type)*(count))
#define ArenaPushArrayZero(arena, type, count) (type*)ArenaPushZero((arena), sizeof(type)*(count))
#define ArenaPushStruct(arena, type) PushArray((arena), (type), 1)
#define ArenaPushStructZero(arena, type) PushArrayZero((arena), (type), 1)

void ArenaPop(MemArena* arena, size_t size);
