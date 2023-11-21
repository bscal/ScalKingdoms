#pragma once

#include "Base.h"
#include "Memory.h"

#include <inttypes.h>

constant_var size_t GENERALPURPOSE_BUCKET_SIZE = 7;

struct MemNode
{
    size_t Size;
    MemNode* Next;
    MemNode* Prev;
};

struct AllocList
{
    MemNode* Head;
    MemNode* Tail;
    size_t Length;
};

// Allocates from an array of FreeLists bases on size. Sizes are
// ceiled to a power of 2. And aligned with lowest alloc size.
// Larger values maybe split buckets if they fit nicely, or just take memory from
// the buffer.
struct GeneralPurposeAllocator
{
    uintptr_t Mem;
    uintptr_t Offset;
    size_t Size;
    AllocList Large;
    AllocList Buckets[GENERALPURPOSE_BUCKET_SIZE];
};

void GeneralPurposeCreate(GeneralPurposeAllocator* allocator, void* buffer, size_t bytes);

void* GeneralPurposeAlloc(GeneralPurposeAllocator* allocator, size_t bytes);
void* GeneralPurposeRealloc(GeneralPurposeAllocator* _RESTRICT_ allocator, void* _RESTRICT_ ptr, size_t bytes);
void GeneralPurposeFree(GeneralPurposeAllocator* _RESTRICT_ allocator, void* _RESTRICT_ ptr);
void GeneralPurposeClearAll(GeneralPurposeAllocator* allocator);

size_t GeneralPurposeGetFreeMemory(GeneralPurposeAllocator* allocator);
