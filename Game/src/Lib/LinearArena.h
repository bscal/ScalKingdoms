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

void LinearArenaCreate(LinearArena* arena, void* buffer, size_t size);

[[nodiscard]] void* LinearArenaAlloc(LinearArena* arena, size_t size);

void LinearArenaReset(LinearArena* arena);

size_t LinearArenaFreeSpace(LinearArena* arena);