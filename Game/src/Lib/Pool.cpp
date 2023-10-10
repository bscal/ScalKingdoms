#include "Pool.h"

#include "Memory.h"

#include <inttypes.h>

void PoolCreate(Pool* pool, SAllocator allocator, size_t numBlocks, size_t blockSize)
{
	PoolCreateAlign(pool, allocator, numBlocks, blockSize, DEFAULT_ALIGNMENT);
}

void PoolCreateAlign(Pool* pool, SAllocator allocator, size_t numBlocks, size_t blockSize,
					 size_t blockAlign)
{
	SAssert(pool);
	*pool = {};
	pool->BlockSize = blockSize;
	pool->BlockAlign = blockAlign;
	pool->NumofBlocks = numBlocks;

	size_t actual_block_size = blockSize + blockAlign;
	pool->AllocationSize = numBlocks * actual_block_size;

	void* data = SAllocAlign(allocator, pool->AllocationSize, (u32)blockAlign);

	// NOTE: Init intrusive freelist
	uintptr_t* end;
	void* curr = data;
	for (size_t block_index = 0; block_index < numBlocks - 1; block_index++)
	{
		uintptr_t* next = (uintptr_t*)curr;
		*next = (uintptr_t)curr + actual_block_size;
		curr = zpl_pointer_add(curr, actual_block_size);
	}

	end = (uintptr_t*)curr;
	*end = (uintptr_t) nullptr;

	pool->Start = data;
	pool->FreeList = data;
}

PoolResizable* PoolResizableCreate(SAllocator allocator, size_t size, size_t numBlocks,
								   size_t blockSize, size_t blockAlign)
{
	size_t actual_block_size = blockSize + blockAlign;
	size_t allocationSize = numBlocks * actual_block_size + sizeof(PoolResizable);

	void* data = SAllocAlign(allocator, allocationSize, (u32)blockAlign);

	PoolResizable* poolResizable = (PoolResizable*)data;

	poolResizable->Pool.BlockSize = blockSize;
	poolResizable->Pool.BlockAlign = blockAlign;
	poolResizable->Pool.NumofBlocks = numBlocks;
	poolResizable->Pool.AllocationSize = numBlocks * actual_block_size;

	// NOTE: Init intrusive freelist
	uintptr_t* end;
	void* listStart = poolResizable + 1;
	void* curr = listStart;
	for (size_t block_index = 0; block_index < numBlocks - 1; ++block_index)
	{
		uintptr_t* next = (uintptr_t*)curr;
		*next = (uintptr_t)curr + actual_block_size;
		curr = (void*)((zpl_u8*)curr + actual_block_size);
	}

	end = (uintptr_t*)curr;
	*end = (uintptr_t) nullptr;

	poolResizable->Pool.Start = listStart;
	poolResizable->Pool.FreeList = listStart;

	return poolResizable;
}
