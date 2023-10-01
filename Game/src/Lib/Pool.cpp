#include "Pool.h"

#include "Memory.h"

#include <inttypes.h>

//! Initialize pool allocator.
void PoolCreate(Pool* pool, SAllocator allocator, size_t size, size_t numBlocks, size_t blockSize)
{
    PoolCreateAlign(pool, allocator, numBlocks, blockSize, DEFAULT_ALIGNMENT);
}

//! Initialize pool allocator with specific block alignment.
void PoolCreateAlign(Pool* pool, SAllocator allocator, size_t numBlocks, size_t blockSize,
                     size_t blockAlign)
{
    size_t actual_block_size, pool_size, block_index;
    void* data, * curr;
    uintptr_t* end;

    pool = {};
    pool->BlockSize = blockSize;
    pool->BlockAlign = blockAlign;
    pool->NumofBlocks = numBlocks;

    actual_block_size = blockSize + blockAlign;
    pool_size = numBlocks * actual_block_size;

    data = SAllocAlign(allocator, pool_size, (u32)blockAlign);

    // NOTE: Init intrusive freelist
    curr = data;
    for (block_index = 0; block_index < numBlocks - 1; block_index++)
    {
        uintptr_t* next = (uintptr_t*) curr;
        *next = (uintptr_t) curr + actual_block_size;
        curr = zpl_pointer_add(curr, actual_block_size);
    }

    end = (uintptr_t*) curr;
    *end = (uintptr_t) nullptr;

    pool->Start = data;
    pool->FreeList = data;
}

SAllocatorProc(PoolAllocatorPorc)
{
    Pool* pool = (Pool*)data;
    void* res = nullptr;

    switch (allocatorType)
    {
    case ALLOCATOR_TYPE_MALLOC:
    {
        uintptr_t next_free;
        SAssert(newSize == pool->BlockSize);
        SAssert(align == pool->BlockAlign);
        SAssert(pool->FreeList != nullptr);

        next_free = *(zpl_uintptr*) pool->FreeList;
        res = pool->FreeList;
        pool->FreeList = (void*) next_free;
        pool->TotalSize += pool->BlockSize;
    } break;

    case ALLOCATOR_TYPE_FREE:
    {
        uintptr_t* next;
        if (!ptr)
            return nullptr;

        next = (uintptr_t*)ptr;
        *next = (uintptr_t) pool->FreeList;
        pool->FreeList = ptr;
        pool->TotalSize -= pool->BlockSize;
    } break;

    case ALLOCATOR_TYPE_REALLOC:
    {
        SError("You cannot resize something allocated by with a pool.");
    }   break;
    }

    return res;
}