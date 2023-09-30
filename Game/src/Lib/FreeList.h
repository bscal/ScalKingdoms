#pragma once

#include "Core.h"
#include "Allocator.h"

#include <inttypes.h>

constant_var size_t FREELIST_BUCKET_SIZE = 8;

struct MemNode
{
    size_t size;
    MemNode* next;
    MemNode* prev;
};

struct AllocList
{
    MemNode* Head;
    MemNode* Tail;
    size_t Length;
};

struct FreeListAllocator
{
    uintptr_t Mem;
    uintptr_t Offset;
    size_t Size;
    AllocList Large;
    AllocList Buckets[FREELIST_BUCKET_SIZE];
};

FreeListAllocator FreelistCreate(SAllocator allocator, size_t bytes);
FreeListAllocator FreelistCreateFromBuffer(void* buffer, size_t bytes);
void FreelistFree(FreeListAllocator* freelist, SAllocator allocator);

void* FreelistAlloc(FreeListAllocator* freelist, size_t bytes);
void* FreelistRealloc(FreeListAllocator* _RESTRICT_ freelist, void* _RESTRICT_ ptr, size_t bytes);
void FreelistFree(FreeListAllocator* _RESTRICT_ freelist, void* _RESTRICT_ ptr);
void FreelistCleanUp(FreeListAllocator* _RESTRICT_ freelist, void** _RESTRICT_ ptrref);
void FreelistReset(FreeListAllocator* freelist);

void* FreelistGetBasePtr(FreeListAllocator* freelist);
size_t FreelistGetFreeMemory(FreeListAllocator* freelist);
