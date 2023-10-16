#pragma once

#include "Core.h"

struct Pool
{
	void* Start;
	void* FreeList;
	size_t BlockSize;
	size_t BlockAlign;
	size_t TotalBlocks;
	size_t AllocationSize;
	size_t NumofBlocks; 
};

struct PoolResizable
{
	Pool Pool;
	PoolResizable* Next;
	SAllocator Allocator;
};

void PoolCreate(Pool* pool, SAllocator allocator, size_t numBlocks, size_t blockSize);

void PoolCreateAlign(Pool* pool, SAllocator allocator, size_t numBlocks,
					 size_t blockSize, size_t blockAlign);

PoolResizable* PoolResizableCreate(SAllocator allocator, size_t size, size_t numBlocks,
								   size_t blockSize, size_t blockAlign);

inline void*
PoolAlloc(Pool* pool)
{
	SAssert(pool);

	void* result = pool->FreeList;
	SAssert(result);
	uintptr_t next = *(uintptr_t*)pool->FreeList;
	pool->FreeList = (void*)next;
	++pool->TotalBlocks;
	return result;
}

inline void*
PoolResizableAlloc(PoolResizable** poolResizablePtr)
{
	SAssert(poolResizablePtr);

	PoolResizable* poolResizable = *poolResizablePtr;
	while (poolResizable)
	{
		Pool* pool = &poolResizable->Pool;

		if (!pool->FreeList && pool->TotalBlocks == pool->NumofBlocks)
		{
			PoolResizable* nextPoolResizable = poolResizable->Next;
			if (!nextPoolResizable)
			{
				PoolResizable* newPool = PoolResizableCreate(poolResizable->Allocator,
															 poolResizable->Pool.AllocationSize,
															 poolResizable->Pool.NumofBlocks,
															 poolResizable->Pool.BlockSize,
															 poolResizable->Pool.BlockAlign);
				SLStackPush(newPool, poolResizable, Next);
			}
			poolResizable = nextPoolResizable;
		}
		else
		{
			return PoolAlloc(pool);
		}
	}
	SError("PoolResizable failed to resize");
	return nullptr;
}

inline void
PoolFree(Pool* pool, void* ptr)
{
	SAssert(pool);

	if (!ptr)
	{
		SError("PoolFree with nullptr ptr");
		return;
	}
	else
	{
		uintptr_t* next = (uintptr_t*)ptr;
		*next = (uintptr_t)pool->FreeList;
		pool->FreeList = ptr;
		--pool->TotalBlocks;
	}
}

inline SAllocatorProc(PoolAllocatorPorc)
{
	Pool* pool = (Pool*)data;
	void* res = nullptr;

	switch (allocatorType)
	{
	case ALLOCATOR_TYPE_MALLOC:
	{
		SAssert(newSize == pool->BlockSize);
		SAssert(align == pool->BlockAlign);
		SAssert(pool->FreeList != nullptr);
		res = PoolAlloc(pool);
	} break;

	case ALLOCATOR_TYPE_FREE:
	{
		PoolFree(pool, ptr);
	} break;

	case ALLOCATOR_TYPE_REALLOC:
	{
		SError("You cannot resize something allocated by with a pool.");
	}   break;
	}

	return res;
}
