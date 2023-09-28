#include "MemoryArena.h"

#include "Utils.h"

#include <stdlib.h>

constant_var size_t MEMARENA_ALIGNMENT = 16;
constant_var size_t MEM_SPLIT_THRESHOLD = MEMARENA_ALIGNMENT * 4;
constant_var size_t MEMARENA_SIZES[MEMARENA_BUCKET_SIZE] =
{
    16, 32, 64, 128, 256, 512, 1024, 2048
};

struct MemNode
{
    size_t size;
    MemNode* next;
    MemNode* prev;
};

internal MemNode* 
SplitMemNode(MemNode* node, size_t bytes)
{
    uintptr_t n = (uintptr_t)node;
    MemNode* r = (MemNode*)(n + (node->size - bytes));
    node->size -= bytes;
    r->size = bytes;

    return r;
}

internal void 
InsertMemNodeBefore(AllocList* list, MemNode* insert, MemNode* curr)
{
    insert->next = curr;

    if (curr->prev == nullptr) list->Head = insert;
    else
    {
        insert->prev = curr->prev;
        curr->prev->next = insert;
    }

    curr->prev = insert;
}

internal MemNode*
RemoveMemNode(AllocList* list, MemNode* node)
{
    if (node->prev != nullptr) node->prev->next = node->next;
    else
    {
        list->Head = node->next;
        if (list->Head != nullptr) list->Head->prev = nullptr;
        else list->Tail = nullptr;
    }

    if (node->next != nullptr) node->next->prev = node->prev;
    else
    {
        list->Tail = node->prev;
        if (list->Tail != nullptr) list->Tail->next = nullptr;
        else list->Head = nullptr;
    }

    list->Length--;

    return node;
}

internal MemNode* 
FindMemNode(AllocList* list, size_t bytes)
{
    for (MemNode* node = list->Head; node != nullptr; node = node->next)
    {
        if (node->size < bytes) continue;

        // Close in size - reduce fragmentation by not splitting
        else if (node->size <= bytes + MEM_SPLIT_THRESHOLD) return RemoveMemNode(list, node);
        else return SplitMemNode(node, bytes);
    }

    return nullptr;
}

internal void 
InsertMemNode(MemArena* memArena, AllocList* list, MemNode* node, bool is_bucket)
{
    if (list->Head == nullptr)
    {
        list->Head = node;
        list->Length++;
    }
    else
    {
        for (MemNode* iter = list->Head; iter != nullptr; iter = iter->next)
        {
            if ((uintptr_t)iter == memArena->Offset)
            {
                memArena->Offset += iter->size;
                RemoveMemNode(list, iter);
                iter = list->Head;

                if (iter == nullptr)
                {
                    list->Head = node;
                    return;
                }
            }

            const uintptr_t inode = (uintptr_t)node;
            const uintptr_t iiter = (uintptr_t)iter;
            const uintptr_t iter_end = iiter + iter->size;
            const uintptr_t node_end = inode + node->size;

            if (iter == node) return;
            else if (iter < node)
            {
                // node was coalesced prior.
                if (iter_end > inode) return;
                else if ((iter_end == inode) && !is_bucket)
                {
                    // if we can coalesce, do so.
                    iter->size += node->size;

                    return;
                }
                else if (iter->next == nullptr)
                {
                    // we reached the end of the free list -> append the node
                    iter->next = node;
                    node->prev = iter;
                    list->Length++;

                    return;
                }
            }
            else if (iter > node)
            {
                // Address sort, lowest to highest aka ascending order.
                if (iiter < node_end) return;
                else if ((iter == list->Head) && !is_bucket)
                {
                    if (iter_end == inode) iter->size += node->size;
                    else if (node_end == iiter)
                    {
                        node->size += list->Head->size;
                        node->next = list->Head->next;
                        node->prev = nullptr;
                        list->Head = node;
                    }
                    else
                    {
                        node->next = iter;
                        node->prev = nullptr;
                        iter->prev = node;
                        list->Head = node;
                        list->Length++;
                    }

                    return;
                }
                else if ((iter_end == inode) && !is_bucket)
                {
                    // if we can coalesce, do so.
                    iter->size += node->size;
                    return;
                }
                else
                {
                    InsertMemNodeBefore(list, node, iter);
                    list->Length++;
                    return;
                }
            }
        }
    }
}

MemArena MemArenaCreate(void* _RESTRICT_ buf, size_t size)
{
    SAssert(buf);
    SAssert(size > 0);

    MemArena memArena = { 0 };

    if ((size == 0) || (buf == nullptr) || (size <= sizeof(MemNode)))
    {
        SError("Creating invalid memory pull!");
    }
    else
    {
        memArena.Size = size;
        memArena.Mem = (uintptr_t)buf;
        memArena.Offset = memArena.Mem + memArena.Size;
    }

    return memArena;
}

void* MemArenaGetBasePtr(MemArena* memArena)
{
    SAssert(memArena);
    if (memArena)
    {
        return (void*)memArena->Mem;
    }
    else
    {
        return nullptr;
    }
}

void* MemArenaAlloc(MemArena* memArena, size_t size)
{
    SAssert(memArena);
    if ((size == 0) || (size > memArena->Size))
    {
        return nullptr;
    }
    else
    {
        MemNode* new_mem = nullptr;
        size_t allocSize = AlignSize(size + sizeof(*new_mem), MEMARENA_ALIGNMENT);
        SAssert(allocSize >= MEMARENA_ALIGNMENT);

        // If the size is small enough, let's check if our buckets has a fitting memory block.
        if (allocSize > MEMARENA_SIZES[MEMARENA_BUCKET_SIZE - 1])
        {
            new_mem = FindMemNode(&memArena->Large, allocSize);
        }
        else
        {
            if (!IsPowerOf2(allocSize))
                allocSize = AlignPowTwo64Ceil(allocSize);

            AllocList* bucketList;
            static_assert(ArrayLength(memArena->Buckets) == 8, "arena buckets count is not 8");
            switch (allocSize)
            {
            case(MEMARENA_SIZES[0]): bucketList = memArena->Buckets + 0; break;
            case(MEMARENA_SIZES[1]): bucketList = memArena->Buckets + 1; break;
            case(MEMARENA_SIZES[2]): bucketList = memArena->Buckets + 2; break;
            case(MEMARENA_SIZES[3]): bucketList = memArena->Buckets + 3; break;
            case(MEMARENA_SIZES[4]): bucketList = memArena->Buckets + 4; break;
            case(MEMARENA_SIZES[5]): bucketList = memArena->Buckets + 5; break;
            case(MEMARENA_SIZES[6]): bucketList = memArena->Buckets + 6; break;
            case(MEMARENA_SIZES[7]): bucketList = memArena->Buckets + 7; break;
            default: SError("Invalid memory size for bucket!");  return nullptr;
            }
            SAssert(bucketList);
            new_mem = FindMemNode(bucketList, allocSize);
            SAssert(new_mem);
        }

        if (new_mem == nullptr)
        {
            // not enough memory to support the size!
            if ((memArena->Offset - allocSize) < memArena->Mem)
            {
                return nullptr;
            }
            else
            {
                // Couldn't allocate from a freelist, allocate from available memArena.
                // Subtract allocation size from the memArena.
                memArena->Offset -= allocSize;

                // Use the available memArena space as the new node.
                new_mem = (MemNode*)memArena->Offset;
                new_mem->size = allocSize;
            }
        }

        // Visual of the allocation block.
        // --------------
        // | mem size   | lowest addr of block
        // | next node  | 12 byte (32-bit) header
        // | prev node  | 24 byte (64-bit) header
        // |------------|
        // |   alloc'd  |
        // |   memory   |
        // |   space    | highest addr of block
        // --------------
        new_mem->next = new_mem->prev = nullptr;
        uint8_t* const _RESTRICT_ final_mem = (uint8_t*)new_mem + sizeof(*new_mem);

        return memset(final_mem, 0, new_mem->size - sizeof(*new_mem));
    }
}

void* MemArenaRealloc(MemArena* _RESTRICT_ memArena, void* ptr, size_t size)
{
    SAssert(memArena);
    if (size > memArena->Size) return nullptr;
    else if (ptr == nullptr) return MemArenaAlloc(memArena, size);
    else if ((uintptr_t)ptr - sizeof(MemNode) < memArena->Mem)return nullptr;
    else
    {
        MemNode* node = (MemNode*)((uint8_t*)ptr - sizeof(*node));
        size_t NODE_SIZE = sizeof(*node);
        uint8_t* resized_block = (uint8_t*)MemArenaAlloc(memArena, size);

        if (resized_block == nullptr)
            return nullptr;
        else
        {
            MemNode* resized = (MemNode*)(resized_block - sizeof(*resized));
            memmove(resized_block, ptr, (node->size > resized->size) ? (resized->size - NODE_SIZE) : (node->size - NODE_SIZE));
            MemArenaFree(memArena, ptr);

            return resized_block;
        }
    }
}

void MemArenaFree(MemArena* _RESTRICT_ memArena, void* ptr)
{
    SAssert(memArena);
    SAssert(ptr);

    uintptr_t p = (uintptr_t)ptr;

    if ((ptr == nullptr) || (p - sizeof(MemNode) < memArena->Mem))
        return;
    else
    {
        // Behind the actual pointer data is the allocation info.
        uintptr_t block = p - sizeof(MemNode);
        MemNode* mem_node = (MemNode*)block;
        //const size_t BUCKET_SLOT = (mem_node->size >> MEMARENA_BUCKET_BITS) - 1;

        // Make sure the pointer data is valid.
        if ((block < memArena->Offset) ||
            ((block - memArena->Mem) > memArena->Size) ||
            (mem_node->size == 0) ||
            (mem_node->size > memArena->Size))
        {
            return;
        }
        // If the mem_node is right at the arena offs, then merge it back to the arena.
        else if (block == memArena->Offset)
        {
            memArena->Offset += mem_node->size;
        }
        else if (mem_node->size <= MEMARENA_SIZES[MEMARENA_BUCKET_SIZE - 1])
        {
            AllocList* bucketList;
            static_assert(ArrayLength(memArena->Buckets) == 8, "arena buckets count is not 8");
            switch (mem_node->size)
            {
            case(MEMARENA_SIZES[0]): bucketList = memArena->Buckets + 0; break;
            case(MEMARENA_SIZES[1]): bucketList = memArena->Buckets + 1; break;
            case(MEMARENA_SIZES[2]): bucketList = memArena->Buckets + 2; break;
            case(MEMARENA_SIZES[3]): bucketList = memArena->Buckets + 3; break;
            case(MEMARENA_SIZES[4]): bucketList = memArena->Buckets + 4; break;
            case(MEMARENA_SIZES[5]): bucketList = memArena->Buckets + 5; break;
            case(MEMARENA_SIZES[6]): bucketList = memArena->Buckets + 6; break;
            case(MEMARENA_SIZES[7]): bucketList = memArena->Buckets + 7; break;
            default: SError("Invalid memory size for bucket!");  return;
            }
            SAssert(bucketList);
            InsertMemNode(memArena, bucketList, mem_node, true);
        }
        else
        {
            InsertMemNode(memArena, &memArena->Large, mem_node, false);
        }
    }
}

void MemArenaCleanUp(MemArena* _RESTRICT_ memArena, void** ptrref)
{
    SAssert(ptrref);
    SAssert(*ptrref);
    MemArenaFree(memArena, *ptrref);
    *ptrref = nullptr;
}

size_t MemArenaGetFreeMemory(MemArena memArena)
{
    size_t total_remaining = memArena.Offset - memArena.Mem;

    for (MemNode* n = memArena.Large.Head; n != nullptr; n = n->next)
        total_remaining += n->size;

    for (size_t i = 0; i < MEMARENA_BUCKET_SIZE; i++)
    {
        for (MemNode* n = memArena.Buckets[i].Head; n != nullptr; n = n->next)
            total_remaining += n->size;
    }
        
    return total_remaining;
}

void MemArenaReset(MemArena* memArena)
{
    SAssert(memArena);

    memArena->Large.Head = memArena->Large.Tail = nullptr;
    memArena->Large.Length = 0;

    for (size_t i = 0; i < MEMARENA_BUCKET_SIZE; i++)
    {
        memArena->Buckets[i].Head = memArena->Buckets[i].Tail = nullptr;
        memArena->Buckets[i].Length = 0;
    }

    memArena->Offset = memArena->Mem + memArena->Size;
}

internal ZPL_ALLOCATOR_PROC(MemArenaAllocatorProc)
{
    MemArena* memArena = cast(MemArena*) allocator_data;
    void* ptr = NULL;

    switch (type) {
    case ZPL_ALLOCATION_ALLOC:
    {
        ptr = MemArenaAlloc(memArena, size);
    } break;
    case ZPL_ALLOCATION_FREE:
    {
        MemArenaFree(memArena, old_memory);
    } break;
    case ZPL_ALLOCATION_RESIZE:
    {
        ptr = MemArenaRealloc(memArena, old_memory, size);
    } break;
    case ZPL_ALLOCATION_FREE_ALL:
        break;
    }

    return ptr;
}

zpl_allocator MemArenaAllocator(MemArena* arena)
{
    zpl_allocator allocator = {};
    allocator.data = arena;
    allocator.proc = MemArenaAllocatorProc;
    return allocator;
}