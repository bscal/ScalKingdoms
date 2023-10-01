#pragma once

#include "Core.h"
#include "Allocator.h"

struct Pool
{
	void* Start;
	void* FreeList;
	size_t BlockSize;
	size_t BlockAlign;
	size_t TotalSize;
	size_t NumofBlocks;
};

//! Initialize pool allocator.
void PoolCreate(Pool* pool, SAllocator allocator, size_t size, size_t numBlocks, size_t blockSize);

//! Initialize pool allocator with specific block alignment.
void PoolCreateAlign(Pool* pool, SAllocator allocator, size_t numBlocks, size_t blockSize,
					 size_t blockAlign);
