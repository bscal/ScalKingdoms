#include "GeneralAllocator.h"

#include "Core.h"

#include <stdlib.h>

constant_var size_t GENERALPURPOSE_BUCKET_SIZES[GENERALPURPOSE_BUCKET_SIZE] =
{
	512, 1024, 2048, 4096, 8192, 16384
};
constant_var size_t GENERALPURPOSE_ALIGNMENT = GENERALPURPOSE_BUCKET_SIZES[0];
constant_var size_t MEM_SPLIT_THRESHOLD = GENERALPURPOSE_BUCKET_SIZES[GENERALPURPOSE_BUCKET_SIZE - 1] << 1;

static_assert(AlignPowTwo64Ceil(AlignSize(1200, GENERALPURPOSE_BUCKET_SIZES[0])) == 2048);
static_assert(ArrayLength(GENERALPURPOSE_BUCKET_SIZES) == GENERALPURPOSE_BUCKET_SIZE, "GENERALPURPOSE_BUCKET_SIZES count is not FREELIST_BUCKET_SIZE");

// Power of 2 to array index
internal _FORCE_INLINE_
u32 ConvertAllocSizeToIndex(size_t allocSize)
{
	SAssert(allocSize >= GENERALPURPOSE_BUCKET_SIZES[0]);
	SAssert(allocSize <= GENERALPURPOSE_BUCKET_SIZES[GENERALPURPOSE_BUCKET_SIZE - 1]);
	size_t allocSizeBucketIndex = allocSize / GENERALPURPOSE_BUCKET_SIZES[0];
	SAssert(allocSizeBucketIndex < GENERALPURPOSE_BUCKET_SIZES[GENERALPURPOSE_BUCKET_SIZE - 1] / allocSizeBucketIndex);

	unsigned long index = 0;
#if _WIN32
	_BitScanForward(&index, (unsigned long)allocSizeBucketIndex);
#else
	// TODO gcc has __builtin_clz maybe look into.

	while (powOf2 > 0)
	{           // until all bits are zero
		if ((powOf2 & 1) == 1)     // check lower bit
			++index;
		powOf2 >>= 1;              // shift bits, removing lower bit
	}
#endif
	return (u32)index;
}

// 65536, 32768 16384, 8192 4096 2048 1024
// 256, 512, 1024, 2048

internal MemNode*
SplitMemNode(MemNode* node, size_t bytes)
{
	uintptr_t n = (uintptr_t)node;
	MemNode* r = (MemNode*)(n + (node->Size - bytes));
	node->Size -= bytes;
	r->Size = bytes;

	return r;
}

internal void
InsertMemNodeBefore(AllocList* list, MemNode* insert, MemNode* curr)
{
	insert->Next = curr;

	if (curr->Prev == nullptr) list->Head = insert;
	else
	{
		insert->Prev = curr->Prev;
		curr->Prev->Next = insert;
	}

	curr->Prev = insert;
}

internal MemNode*
RemoveMemNode(AllocList* list, MemNode* node)
{
	if (node->Prev != nullptr) node->Prev->Next = node->Next;
	else
	{
		list->Head = node->Next;
		if (list->Head != nullptr) list->Head->Prev = nullptr;
		else list->Tail = nullptr;
	}

	if (node->Next != nullptr) node->Next->Prev = node->Prev;
	else
	{
		list->Tail = node->Prev;
		if (list->Tail != nullptr) list->Tail->Next = nullptr;
		else list->Head = nullptr;
	}

	list->Length--;

	return node;
}

internal MemNode*
FindMemNode(AllocList* list, size_t bytes)
{
	for (MemNode* node = list->Head; node != nullptr; node = node->Next)
	{
		if (node->Size < bytes)
			continue;

		// Try to reduce fragmentation. But can waste memory if allocating slightly above the largest
		// freelist.
		if (node->Size <= bytes + MEM_SPLIT_THRESHOLD)
			return RemoveMemNode(list, node);
		else
			return SplitMemNode(node, bytes);
	}

	return nullptr;
}

internal void
InsertMemNode(GeneralPurposeAllocator* freelist, AllocList* list, MemNode* node, bool is_bucket)
{
	if (list->Head == nullptr)
	{
		list->Head = node;
		list->Length++;
	}
	else
	{
		for (MemNode* iter = list->Head; iter != nullptr; iter = iter->Next)
		{
			if ((uintptr_t)iter == freelist->Offset)
			{
				freelist->Offset += iter->Size;
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
			const uintptr_t iter_end = iiter + iter->Size;
			const uintptr_t node_end = inode + node->Size;

			if (iter == node) return;
			else if (iter < node)
			{
				// node was coalesced prior.
				if (iter_end > inode) return;
				else if ((iter_end == inode) && !is_bucket)
				{
					// if we can coalesce, do so.
					iter->Size += node->Size;

					return;
				}
				else if (iter->Next == nullptr)
				{
					// we reached the end of the free list -> append the node
					iter->Next = node;
					node->Prev = iter;
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
					if (iter_end == inode) iter->Size += node->Size;
					else if (node_end == iiter)
					{
						node->Size += list->Head->Size;
						node->Next = list->Head->Next;
						node->Prev = nullptr;
						list->Head = node;
					}
					else
					{
						node->Next = iter;
						node->Prev = nullptr;
						iter->Prev = node;
						list->Head = node;
						list->Length++;
					}

					return;
				}
				else if ((iter_end == inode) && !is_bucket)
				{
					// if we can coalesce, do so.
					iter->Size += node->Size;
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

GeneralPurposeAllocator FreelistCreate(SAllocator allocator, size_t bytes)
{
	SAssert(IsAllocatorValid(allocator));
	SAssert(bytes > 0);

	GeneralPurposeAllocator freelist = { 0 };

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

void GeneralPurposeCreate(GeneralPurposeAllocator* allocator, void* buffer, size_t bytes)
{
	SAssert(allocator);
	SAssert(buffer);
	SAssert(bytes > 0);

	*allocator = {};

	if ((bytes == 0) || (bytes <= sizeof(MemNode)) || !buffer)
	{
		SError("Invalid general purpose allocator");
	}
	else
	{
		allocator->Size = bytes;
		allocator->Mem = (uintptr_t)buffer;
		allocator->Offset = allocator->Mem + allocator->Size;
	}
}

void* GeneralPurposeAlloc(GeneralPurposeAllocator* allocator, size_t size)
{
	SAssert(allocator);
	SAssert(size > 0);
	SAssert(size < allocator->Size);

	MemNode* newMemNode = nullptr;
	size_t allocSize = AlignSize(size + sizeof(MemNode), GENERALPURPOSE_ALIGNMENT);
	SAssert(allocSize >= GENERALPURPOSE_ALIGNMENT);

	// We check large allocations right away
	if (allocSize > GENERALPURPOSE_BUCKET_SIZES[GENERALPURPOSE_BUCKET_SIZE - 1])
	{
		newMemNode = FindMemNode(&allocator->Large, allocSize);
	}
	else
	{
		allocSize = AlignPowTwo64Ceil(allocSize);
		u32 bucketIndex = ConvertAllocSizeToIndex(allocSize);
		AllocList* bucketList = allocator->Buckets + bucketIndex;
		if (bucketList->Head)
			newMemNode = RemoveMemNode(bucketList, bucketList->Head);
	}

	if (!newMemNode)
	{
		// not enough memory to support the size!
		if ((allocator->Offset - allocSize) < allocator->Mem)
		{
			SError("[ Memory ] Memory Arena is out of memory!");
			return nullptr;
		}
		else
		{
			// TODO Maybe alloc new allocations from large list?

			// Couldn't allocate from a freelist, allocate from available freelist.
			// Subtract allocation size from the freelist.
			allocator->Offset -= allocSize;

			// Use the available freelist space as the new node.
			newMemNode = (MemNode*)allocator->Offset;
			newMemNode->Size = allocSize;
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
	newMemNode->Next = newMemNode->Prev = nullptr;
	uint8_t* final_mem = (uint8_t*)newMemNode + sizeof(*newMemNode);
	SAssert(final_mem);
	return final_mem;
}

void* GeneralPurposeRealloc(GeneralPurposeAllocator* _RESTRICT_ freelist, void* _RESTRICT_ ptr, size_t size)
{
	SAssert(freelist);
	SAssert(size <= freelist->Size);

	if (!ptr)
	{
		return GeneralPurposeAlloc(freelist, size);
	}
	else // Handles a full free and alloc here
	{
		uintptr_t block = (uintptr_t)ptr - sizeof(MemNode);
		SAssert(block >= freelist->Offset);
		SAssert(block - freelist->Mem <= freelist->Size);
		// REVISIT maybe just use the asserts
		if (block < freelist->Offset || block - freelist->Mem > freelist->Size)
		{
			SError("Invalid pointer");
			return nullptr;
		}

		MemNode* node = (MemNode*)block;
		SAssert(node->Size != 0);
		SAssert(node->Size < freelist->Size);

		if (size < node->Size)
			return ptr;

		uint8_t* resized_block = (uint8_t*)GeneralPurposeAlloc(freelist, size);
		SAssert(resized_block);

		if (resized_block)
		{
			MemNode* resized = (MemNode*)(resized_block - sizeof(MemNode));
			SMemMove(resized_block, ptr, (node->Size > resized->Size)
					? (resized->Size - sizeof(MemNode)) : (node->Size - sizeof(MemNode)));
			GeneralPurposeFree(freelist, ptr);
			SAssert(ptr);
			return resized_block;
		}
	}
	return nullptr;
}

void GeneralPurposeFree(GeneralPurposeAllocator* _RESTRICT_ freelist, void* _RESTRICT_ ptr)
{
	static_assert(ArrayLength(GENERALPURPOSE_BUCKET_SIZES) == GENERALPURPOSE_BUCKET_SIZE, "GENERALPURPOSE_BUCKET_SIZES Count is not FREELIST_BUCKET_SIZE");

	SAssert(freelist);

	if (!ptr)
		return;
	else
	{
		// Behind the actual pointer data is the allocation info.
		uintptr_t block = (uintptr_t)ptr - sizeof(MemNode);
		MemNode* mem_node = (MemNode*)block;

		SAssert(block >= freelist->Offset);
		SAssert(block - freelist->Mem <= freelist->Size);
		SAssert(mem_node->Size > 0);
		SAssert(mem_node->Size < freelist->Size);

		// REVISIT maybe just use the asserts
		if (block < freelist->Offset || block - freelist->Mem > freelist->Size)
		{
			SError("Invalid pointer");
			return;
		}

		// If the mem_node is right at the arena offs, then merge it back to the arena.
		if (block == freelist->Offset)
		{
			freelist->Offset += mem_node->Size;
		}
		else if (mem_node->Size <= GENERALPURPOSE_BUCKET_SIZES[GENERALPURPOSE_BUCKET_SIZE - 1])
		{
			u32 bucketIndex = ConvertAllocSizeToIndex(mem_node->Size);
			AllocList* bucketList = freelist->Buckets + bucketIndex;
			InsertMemNode(freelist, bucketList, mem_node, true);
		}
		else
		{
			InsertMemNode(freelist, &freelist->Large, mem_node, false);
		}
	}
}

size_t GeneralPurposeGetFreeMemory(GeneralPurposeAllocator* freelist)
{
	SAssert(freelist);

	size_t total_remaining = freelist->Offset - freelist->Mem;

	for (MemNode* n = freelist->Large.Head; n != nullptr; n = n->Next)
		total_remaining += n->Size;

	for (size_t i = 0; i < GENERALPURPOSE_BUCKET_SIZE; i++)
	{
		for (MemNode* n = freelist->Buckets[i].Head; n != nullptr; n = n->Next)
			total_remaining += n->Size;
	}

	return total_remaining;
}

void GeneralPurposeClearAll(GeneralPurposeAllocator* freelist)
{
	SAssert(freelist);

	freelist->Large.Head = freelist->Large.Tail = nullptr;
	freelist->Large.Length = 0;

	for (size_t i = 0; i < GENERALPURPOSE_BUCKET_SIZE; i++)
	{
		freelist->Buckets[i].Head = freelist->Buckets[i].Tail = nullptr;
		freelist->Buckets[i].Length = 0;
	}

	freelist->Offset = freelist->Mem + freelist->Size;
}
