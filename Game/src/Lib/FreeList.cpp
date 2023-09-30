#include "FreeList.h"

#include "Memory.h"
#include "Utils.h"

#include <stdlib.h>

constant_var size_t FREELIST_ALIGNMENT = 16;
constant_var size_t MEM_SPLIT_THRESHOLD = FREELIST_ALIGNMENT * 2;
constant_var size_t FREELIST_BUCKET_SIZES[FREELIST_BUCKET_SIZE] =
{
    16, 32, 64, 128, 256, 512, 1024, 2048
};


// 65536, 32768 16384, 8192 4096 2048 1024

static_assert(ArrayLength(FREELIST_BUCKET_SIZES) == 8, "MEMARENA_SIZES count is not 8");

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
InsertMemNode(FreeListAllocator* freelist, AllocList* list, MemNode* node, bool is_bucket)
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
            if ((uintptr_t)iter == freelist->Offset)
            {
                freelist->Offset += iter->size;
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

FreeListAllocator FreelistCreate(SAllocator allocator, size_t bytes)
{
    SAssert(IsAllocatorValid(allocator));
    SAssert(bytes > 0);

    FreeListAllocator freelist = { 0 };

    if ((bytes == 0) || (bytes <= sizeof(MemNode)))
    {
        SError("General purpose allocator, invalid size");
    }
    else
    {
        void* buffer = SAlloc(allocator, bytes);
        if (!buffer)
        {
            SError("General purpose allocator, buffer is null");
            return freelist;
        }

        freelist.Size = bytes;
        freelist.Mem = (uintptr_t)buffer;
        freelist.Offset = freelist.Mem + freelist.Size;
    }
    return freelist;
}

FreeListAllocator FreelistCreateFromBuffer(void* buffer, size_t bytes)
{
    SAssert(buffer);
    SAssert(bytes > 0);

    FreeListAllocator freelist = { 0 };

    if ((bytes == 0) || (bytes <= sizeof(MemNode)) || !buffer)
    {
        SError("Invalid general purpose allocator");
    }
    else
    {
        freelist.Size = bytes;
        freelist.Mem = (uintptr_t)buffer;
        freelist.Offset = freelist.Mem + freelist.Size;
    }

    return freelist;
}

void FreelistFree(FreeListAllocator* freelist, SAllocator allocator)
{
    SAssert(freelist);
    SAssert(IsAllocatorValid(allocator));
    if (freelist && freelist->Mem)
    {
        SFree(allocator, (void*)freelist->Mem);
    }
}

void* FreelistGetBasePtr(FreeListAllocator* freelist)
{
    SAssert(freelist);
    return (freelist) ? (void*)freelist->Mem : nullptr;
}

void* FreelistAlloc(FreeListAllocator* freelist, size_t size)
{
    static_assert(ArrayLength(freelist->Buckets) == 8, "freelist->Buckets count is not 8");
    SAssert(freelist);
    SAssert(size > 0);
    SAssert(size < freelist->Size);

    MemNode* new_mem = nullptr;
    size_t allocSize = AlignSize(size + sizeof(*new_mem), FREELIST_ALIGNMENT);
    SAssert(allocSize >= FREELIST_ALIGNMENT);

    // Goes straight to large allocations or small allocation bucket lists 
    if (allocSize > FREELIST_BUCKET_SIZES[FREELIST_BUCKET_SIZE - 1])
    {
        new_mem = FindMemNode(&freelist->Large, allocSize);
    }
    else
    {
        SAssert(allocSize >= FREELIST_BUCKET_SIZES[0]);
        SAssert(allocSize <= FREELIST_BUCKET_SIZES[FREELIST_BUCKET_SIZE - 1]);
        if (!IsPowerOf2(allocSize))
            allocSize = AlignPowTwo64Ceil(allocSize);

        AllocList* bucketList;
        switch (allocSize)
        {
        case(FREELIST_BUCKET_SIZES[0]): bucketList = freelist->Buckets + 0; break;
        case(FREELIST_BUCKET_SIZES[1]): bucketList = freelist->Buckets + 1; break;
        case(FREELIST_BUCKET_SIZES[2]): bucketList = freelist->Buckets + 2; break;
        case(FREELIST_BUCKET_SIZES[3]): bucketList = freelist->Buckets + 3; break;
        case(FREELIST_BUCKET_SIZES[4]): bucketList = freelist->Buckets + 4; break;
        case(FREELIST_BUCKET_SIZES[5]): bucketList = freelist->Buckets + 5; break;
        case(FREELIST_BUCKET_SIZES[6]): bucketList = freelist->Buckets + 6; break;
        case(FREELIST_BUCKET_SIZES[7]): bucketList = freelist->Buckets + 7; break;
        default: SError("Invalid memory size for bucket!");  return nullptr;
        }
        new_mem = FindMemNode(bucketList, allocSize);
    }

    if (new_mem == nullptr)
    {
        // not enough memory to support the size!
        if ((freelist->Offset - allocSize) < freelist->Mem)
        {
            SError("[ Memory ] Memory Arena is out of memory!");
            return nullptr;
        }
        else
        {
            // Couldn't allocate from a freelist, allocate from available freelist.
            // Subtract allocation size from the freelist.
            freelist->Offset -= allocSize;

            // Use the available freelist space as the new node.
            new_mem = (MemNode*)freelist->Offset;
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
    uint8_t* final_mem = (uint8_t*)new_mem + sizeof(*new_mem);
    return final_mem;
    //return memset(final_mem, 0, new_mem->size - sizeof(*new_mem));
}

void* FreelistRealloc(FreeListAllocator* _RESTRICT_ freelist, void* _RESTRICT_ ptr, size_t size)
{
    SAssert(freelist);
    SAssert(size <= freelist->Size);

    if (ptr == nullptr)
    {
        return FreelistAlloc(freelist, size);
    }
    else if ((uintptr_t)ptr - sizeof(MemNode) < freelist->Mem)
    {
        SError("Invalid ptr");
        return nullptr;
    }
    else // Handles a full free and alloc here
    {
        MemNode* node = (MemNode*)((uint8_t*)ptr - sizeof(*node));
        size_t NODE_SIZE = sizeof(*node);
        uint8_t* resized_block = (uint8_t*)FreelistAlloc(freelist, size);

        if (resized_block == nullptr)
            return nullptr;
        else
        {
            MemNode* resized = (MemNode*)(resized_block - sizeof(*resized));
            memmove(resized_block, ptr, (node->size > resized->size) ? (resized->size - NODE_SIZE) : (node->size - NODE_SIZE));
            FreelistFree(freelist, ptr);

            return resized_block;
        }
    }
}

void FreelistFree(FreeListAllocator* _RESTRICT_ freelist, void* _RESTRICT_ ptr)
{
    static_assert(ArrayLength(FREELIST_BUCKET_SIZES) == 8, "MEMARENA_SIZES count is not 8");
    SAssert(freelist);
    SAssert(ptr);

    uintptr_t p = (uintptr_t)ptr;

    if ((ptr == nullptr) || (p - sizeof(MemNode) < freelist->Mem))
        return;
    else
    {
        // Behind the actual pointer data is the allocation info.
        uintptr_t block = p - sizeof(MemNode);
        MemNode* mem_node = (MemNode*)block;
        //const size_t BUCKET_SLOT = (mem_node->size >> MEMARENA_BUCKET_BITS) - 1;

        // Make sure the pointer data is valid.
        if ((block < freelist->Offset) ||
            ((block - freelist->Mem) > freelist->Size) ||
            (mem_node->size == 0) ||
            (mem_node->size > freelist->Size))
        {
            return;
        }
        // If the mem_node is right at the arena offs, then merge it back to the arena.
        else if (block == freelist->Offset)
        {
            freelist->Offset += mem_node->size;
        }
        else if (mem_node->size <= FREELIST_BUCKET_SIZES[FREELIST_BUCKET_SIZE - 1])
        {
            SAssert(mem_node->size >= FREELIST_BUCKET_SIZES[0]);
            SAssert(mem_node->size <= FREELIST_BUCKET_SIZES[FREELIST_BUCKET_SIZE - 1]);

            AllocList* bucketList;
            switch (mem_node->size)
            {
            case(FREELIST_BUCKET_SIZES[0]): bucketList = freelist->Buckets + 0; break;
            case(FREELIST_BUCKET_SIZES[1]): bucketList = freelist->Buckets + 1; break;
            case(FREELIST_BUCKET_SIZES[2]): bucketList = freelist->Buckets + 2; break;
            case(FREELIST_BUCKET_SIZES[3]): bucketList = freelist->Buckets + 3; break;
            case(FREELIST_BUCKET_SIZES[4]): bucketList = freelist->Buckets + 4; break;
            case(FREELIST_BUCKET_SIZES[5]): bucketList = freelist->Buckets + 5; break;
            case(FREELIST_BUCKET_SIZES[6]): bucketList = freelist->Buckets + 6; break;
            case(FREELIST_BUCKET_SIZES[7]): bucketList = freelist->Buckets + 7; break;
            default: SError("Invalid memory size for bucket!");  return;
            }
            InsertMemNode(freelist, bucketList, mem_node, true);
        }
        else
        {
            InsertMemNode(freelist, &freelist->Large, mem_node, false);
        }
    }
}

void FreelistCleanUp(FreeListAllocator* _RESTRICT_ freelist, void** _RESTRICT_ ptrref)
{
    SAssert(ptrref);
    SAssert(*ptrref);
    FreelistFree(freelist, *ptrref);
    *ptrref = nullptr;
}

size_t FreelistGetFreeMemory(FreeListAllocator* freelist)
{
    size_t total_remaining = freelist->Offset - freelist->Mem;

    for (MemNode* n = freelist->Large.Head; n != nullptr; n = n->next)
        total_remaining += n->size;

    for (size_t i = 0; i < FREELIST_BUCKET_SIZE; i++)
    {
        for (MemNode* n = freelist->Buckets[i].Head; n != nullptr; n = n->next)
            total_remaining += n->size;
    }
        
    return total_remaining;
}

void FreelistReset(FreeListAllocator* freelist)
{
    SAssert(freelist);

    freelist->Large.Head = freelist->Large.Tail = nullptr;
    freelist->Large.Length = 0;

    for (size_t i = 0; i < FREELIST_BUCKET_SIZE; i++)
    {
        freelist->Buckets[i].Head = freelist->Buckets[i].Tail = nullptr;
        freelist->Buckets[i].Length = 0;
    }

    freelist->Offset = freelist->Mem + freelist->Size;
}
