#pragma once

#include "Core.h"

#include <inttypes.h>

constexpr global size_t MEMARENA_BUCKET_SIZE = 8;

struct MemNode;

struct AllocList
{
    MemNode* Head;
    MemNode* Tail;
    size_t Length;
};

// Memory pool
struct MemArena
{
    uintptr_t Mem;
    uintptr_t Offset;
    size_t Size;
    AllocList Large;
    AllocList Buckets[MEMARENA_BUCKET_SIZE];
};

MemArena MemArenaCreate(void* _RESTRICT_ buf, size_t bytes);

void* MemArenaAlloc(MemArena* memArena, size_t bytes);
void* MemArenaRealloc(MemArena* _RESTRICT_ memArena, void* ptr, size_t bytes);
void MemArenaFree(MemArena* _RESTRICT_ memArena, void* ptr);
void MemArenaCleanUp(MemArena* _RESTRICT_ memArena, void** ptrref);
void MemArenaReset(MemArena* memArena);

void* MemArenaGetBasePtr(MemArena* memArena);
size_t MemArenaGetFreeMemory(const MemArena memArena);

zpl_allocator MemArenaAllocator(MemArena* arena);
