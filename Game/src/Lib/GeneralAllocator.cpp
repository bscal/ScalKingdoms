#include "GeneralAllocator.h"

#include "Memory.h"
#include "Utils.h"

#include <stdlib.h>

constant_var size_t GENERALPURPOSE_BUCKET_SIZES[GENERALPURPOSE_BUCKET_SIZE] =
{
	512, 1024, 2048, 4096, 8192, 16384
};
constant_var size_t GENERALPURPOSE_ALIGNMENT = GENERALPURPOSE_BUCKET_SIZES[0];
constant_var size_t MEM_SPLIT_THRESHOLD = GENERALPURPOSE_BUCKET_SIZES[GENERALPURPOSE_BUCKET_SIZE - 1];

static_assert(AlignPowTwo64Ceil(AlignSize(1200, GENERALPURPOSE_BUCKET_SIZES[0])) == 2048);
static_assert(ArrayLength(GENERALPURPOSE_BUCKET_SIZES) == GENERALPURPOSE_BUCKET_SIZE, "GENERALPURPOSE_BUCKET_SIZES count is not FREELIST_BUCKET_SIZE");


// 65536, 32768 16384, 8192 4096 2048 1024

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
		if (node->size < bytes)
			continue;

		// Close in size - reduce fragmentation by not splitting
		else if (node->size <= bytes + MEM_SPLIT_THRESHOLD) 
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

	MemNode* new_mem = nullptr;
	size_t allocSize = AlignSize(size + sizeof(MemNode), GENERALPURPOSE_ALIGNMENT);
	SAssert(allocSize >= GENERALPURPOSE_ALIGNMENT);

	// We check large allocations right away
	if (allocSize > GENERALPURPOSE_BUCKET_SIZES[GENERALPURPOSE_BUCKET_SIZE - 1])
		new_mem = FindMemNode(&allocator->Large, allocSize);
	else
	{
		if (!IsPowerOf2(allocSize))
			allocSize = AlignPowTwo64Ceil(allocSize);

		SAssert(allocSize >= GENERALPURPOSE_BUCKET_SIZES[0]);
		SAssert(allocSize <= GENERALPURPOSE_BUCKET_SIZES[GENERALPURPOSE_BUCKET_SIZE - 1]);

		static_assert(ArrayLength(allocator->Buckets) == GENERALPURPOSE_BUCKET_SIZE, "ArrayLength(allocator->Buckets) is not FREELIST_BUCKET_SIZE");
		AllocList* bucketList;
		switch (allocSize)
		{
		case(GENERALPURPOSE_BUCKET_SIZES[0]): bucketList = allocator->Buckets + 0; break;
		case(GENERALPURPOSE_BUCKET_SIZES[1]): bucketList = allocator->Buckets + 1; break;
		case(GENERALPURPOSE_BUCKET_SIZES[2]): bucketList = allocator->Buckets + 2; break;
		case(GENERALPURPOSE_BUCKET_SIZES[3]): bucketList = allocator->Buckets + 3; break;
		case(GENERALPURPOSE_BUCKET_SIZES[4]): bucketList = allocator->Buckets + 4; break;
		case(GENERALPURPOSE_BUCKET_SIZES[5]): bucketList = allocator->Buckets + 5; break;
		default:
			SError("Invalid memory size for bucket!"); return nullptr;
		}

		if (bucketList->Head)
			new_mem = RemoveMemNode(bucketList, bucketList->Head);
	}

	if (!new_mem)
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
			new_mem = (MemNode*)allocator->Offset;
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
		SAssert((uintptr_t)ptr >= freelist->Offset);
		SAssert((uintptr_t)ptr - freelist->Mem <= freelist->Size);

		MemNode* node = (MemNode*)((uint8_t*)ptr - sizeof(*node));
		SAssert(node->size != 0);
		SAssert(node->size < freelist->Size);
		//if (node->size > size + sizeof(MemNode))
			//return ptr;

		uint8_t* resized_block = (uint8_t*)GeneralPurposeAlloc(freelist, size);
		SAssert(resized_block);

		if (!resized_block)
			return nullptr;
		else
		{
			MemNode* resized = (MemNode*)(resized_block - sizeof(MemNode));
			memmove(resized_block, ptr, (node->size > resized->size) 
					? (resized->size - sizeof(MemNode)) : (node->size - sizeof(MemNode)));
			GeneralPurposeFree(freelist, ptr);
			SAssert(ptr);
			return resized_block;
		}
	}
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
		SAssert(mem_node->size > 0);
		SAssert(mem_node->size < freelist->Size);

		// If the mem_node is right at the arena offs, then merge it back to the arena.
		if (block == freelist->Offset)
		{
			freelist->Offset += mem_node->size;
		}
		else if (mem_node->size <= GENERALPURPOSE_BUCKET_SIZES[GENERALPURPOSE_BUCKET_SIZE - 1])
		{
			SAssert(mem_node->size >= GENERALPURPOSE_BUCKET_SIZES[0]);
			SAssert(mem_node->size <= GENERALPURPOSE_BUCKET_SIZES[GENERALPURPOSE_BUCKET_SIZE - 1]);

			static_assert(ArrayLength(freelist->Buckets) == GENERALPURPOSE_BUCKET_SIZE, "ArrayLength(freelist->Buckets) is not FREELIST_BUCKET_SIZE");
			AllocList* bucketList;
			switch (mem_node->size)
			{
			case(GENERALPURPOSE_BUCKET_SIZES[0]): bucketList = freelist->Buckets + 0; break;
			case(GENERALPURPOSE_BUCKET_SIZES[1]): bucketList = freelist->Buckets + 1; break;
			case(GENERALPURPOSE_BUCKET_SIZES[2]): bucketList = freelist->Buckets + 2; break;
			case(GENERALPURPOSE_BUCKET_SIZES[3]): bucketList = freelist->Buckets + 3; break;
			case(GENERALPURPOSE_BUCKET_SIZES[4]): bucketList = freelist->Buckets + 4; break;
			case(GENERALPURPOSE_BUCKET_SIZES[5]): bucketList = freelist->Buckets + 5; break;
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

size_t GeneralPurposeGetFreeMemory(GeneralPurposeAllocator* freelist)
{
	SAssert(freelist);

	size_t total_remaining = freelist->Offset - freelist->Mem;

	for (MemNode* n = freelist->Large.Head; n != nullptr; n = n->next)
		total_remaining += n->size;

	for (size_t i = 0; i < GENERALPURPOSE_BUCKET_SIZE; i++)
	{
		for (MemNode* n = freelist->Buckets[i].Head; n != nullptr; n = n->next)
			total_remaining += n->size;
	}

	return total_remaining;
}

void GeneralPurposeReset(GeneralPurposeAllocator* freelist)
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
