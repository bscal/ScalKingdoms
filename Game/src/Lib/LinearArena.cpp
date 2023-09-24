#include "LinearArena.h"

#include "Utils.h"

LinearArena LinearArenaCreate(Allocator allocator, size_t size)
{
    if (!IsAllocatorValid(allocator))
    {
        SError("LinearArena allocator is invalid");
        return {};
    }
    if (size == 0)
    {
        SError("LinearArena size cannot be 0");
        return {};
    }

    void* buffer = SMalloc(allocator, size);

    LinearArena arena;
    arena.Size = size;
    arena.Memory = (uintptr_t)buffer;
    arena.Front = arena.Memory;
    arena.Back = arena.Memory + size;

    SAssert(arena.Memory);
    return arena;
}


void LinearArenaFree(Allocator allocator, LinearArena* arena)
{
    SAssert(arena);
    SAssert(arena->Memory);
    SAssert(arena->Size > 0);
    SAssert(IsAllocatorValid(allocator));

    SFree(allocator, (void*)arena->Memory);
}

[[nodiscard]] void* LinearArenaAlloc(LinearArena* arena, size_t size)
{
    SAssert(arena);
    SAssert(arena->Memory);

    if (size == 0)
    {
        SError("LinearArena alloc size is 0");
        return nullptr;
    }
    else
    {
        size_t alignedSize = AlignSize(size, sizeof(uintptr_t));
        uintptr_t newFront = arena->Front + alignedSize;
        if (newFront >= arena->Back)
        {
            SWarn("LinearArena alloc Front overflowing Back");
            return nullptr;
        }
        else
        {
            uint8_t* ptr = (uint8_t*)arena->Front;
            arena->Front = newFront;
            return ptr;
        }
    }
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
