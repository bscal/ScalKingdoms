#pragma once

#include "Core.h"
#include "Memory.h"

#include <inttypes.h>

struct LinearArena
{
    uintptr_t Memory;
    uintptr_t Front;
    uintptr_t Back;
    size_t Size;
};

LinearArena LinearArenaCreate(Allocator allocator, size_t size);

void LinearArenaFree(Allocator allocator, LinearArena* arena);

[[nodiscard]] void* LinearArenaAlloc(LinearArena* arena, size_t size);

void LinearArenaReset(LinearArena* arena);

size_t LinearArenaFreeSpace(LinearArena* arena);