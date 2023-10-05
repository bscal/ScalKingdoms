#pragma once

#include "Core.h"
#include "Memory.h"
#include "Utils.h"

#include <inttypes.h>

struct LinearArena
{
	uintptr_t Memory;
	uintptr_t Front;
	uintptr_t Back;
	size_t Size;
};

inline void LinearArenaCreate(LinearArena* arena, void* buffer, size_t size);
inline [[nodiscard]] void* LinearArenaAlloc(LinearArena* arena, size_t size);
inline void LinearArenaReset(LinearArena* arena);
inline size_t LinearArenaFreeSpace(LinearArena* arena);

// *******************************************************************

void LinearArenaCreate(LinearArena* arena, void* buffer, size_t size)
{
	SAssert(arena);
	SAssert(buffer);
	if (!arena || !buffer || size == 0)
	{
		SError("Could not initialize linear arena");
	}
	arena->Size = size;
	arena->Memory = (uintptr_t)buffer;
	arena->Front = arena->Memory;
	arena->Back = arena->Memory + size;
}

[[nodiscard]] void* LinearArenaAlloc(LinearArena* arena, size_t size)
{
	SAssert(arena);
	SAssert(arena->Memory);
	SAssert(size > 0);

	size_t alignedSize = AlignSize(size, DEFAULT_ALIGNMENT);
	uintptr_t newFront = arena->Front + alignedSize;

	void* ptr = (void*)arena->Front;
	arena->Front = newFront;

	if (newFront >= arena->Back)
	{
		SError("LinearArena alloc Front overflowing Back");
		ptr = nullptr;
	}

	return ptr;
}

void LinearArenaReset(LinearArena* arena)
{
	SAssert(arena);
	SAssert(arena->Memory);
	arena->Front = arena->Memory;
}

size_t LinearArenaFreeSpace(LinearArena* arena)
{
	SAssert(arena);
	return (arena->Memory) ? arena->Back - arena->Front : 0;
}
