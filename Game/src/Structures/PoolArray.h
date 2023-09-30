#pragma once

#include "Core.h"
#include "Memory.h"
#include "Utils.h"

template<typename T, int ElementsPerBlock>
struct PoolArray
{
	T** MemoryBlocks;
	int Head;
	int Tail;
	int BlockCount;
	int TotalElements;
	SAllocator Alloc;

	void Initialize(SAllocator SAllocator, int startBlockCapacity)
	{
		static_assert(ElementsPerBlock > 0);
		
		SAssert(IsAllocatorValid(SAllocator));
		
		MemoryBlocks = nullptr;
		Alloc = SAllocator;
		Head = 0;
		Tail = 0;
		BlockCount = 0;
		TotalElements = 0;

		AllocBlock(startBlockCapacity);
	}

	void Free()
	{
		SAssert(MemoryBlocks);
		SAssert(IsAllocatorValid(Alloc));
		for (int i = 0; i < BlockCount; ++i)
		{
			SFree(Alloc, MemoryBlocks[i]);
		}
		SFree(Alloc, MemoryBlocks);
	}

	void AllocBlock(int numOfBlocks)
	{
		if (numOfBlocks == 0)
			return;

		int newBlockCount = BlockCount + numOfBlocks;
		if (newBlockCount >= BlockCount)
		{
			size_t oldSize = sizeof(T*) * BlockCount;
			size_t newSize = sizeof(T*) * (newBlockCount);
			MemoryBlocks = (T**)SRealloc(Alloc, MemoryBlocks, newSize, oldSize);
		}

		for (int i = 0; i < numOfBlocks; ++i)
		{
			int idx = BlockCount + i;
			size_t size = ElementsPerBlock * SizeOfElement();
			MemoryBlocks[idx] = (T*)SAlloc(Alloc, size);
		}

		BlockCount = newBlockCount;
	}

	constexpr _FORCE_INLINE_ size_t SizeOfElement()
	{
		if constexpr (sizeof(T) < sizeof(void*))
			return sizeof(void*);
		else
			return sizeof(T);
	}

	int GetNext()
	{
		int res;
		if (!Head)
		{
			int tailIdx = Tail / ElementsPerBlock;
			if (tailIdx >= BlockCount)
				AllocBlock(1);

			res = Tail;
			++Tail;
		}
		else
		{
			res = Head;
			Head = (int)*At(Head);
		}
		++TotalElements;
		return res;
	}

	T* GetNextValue()
	{
		int idx = GetNext();
		return At(idx);
	}

	T* At(int idx)
	{
		SAssert(MemoryBlocks);
		int blockIdx = idx / ElementsPerBlock;
		int blockLocalIdx = idx % ElementsPerBlock;
		SAssert(blockIdx >= 0);
		SAssert(blockIdx < BlockCount);
		SAssert(blockLocalIdx >= 0);
		SAssert(blockLocalIdx < ElementsPerBlock);
		SAssert(MemoryBlocks + blockIdx);
		return &MemoryBlocks[blockIdx][blockLocalIdx];
	}

	void Return(int idx)
	{
		T* newHead = At(idx);
		*newHead = Head;
		Head = idx;
		--TotalElements;
	}

	void ReturnPtr(T* ptr)
	{
		for (int block = 0; block < BlockCount; ++block)
		{
			for (int element = 0; element < ElementsPerBlock; ++element)
			{
				T* elementPtr = &MemoryBlocks[block][element];
				if (elementPtr == ptr)
				{
					*elementPtr = Head;
					Head = element + block * ElementsPerBlock;
					--TotalElements;
				}
			}
		}
	}
};

inline int Test()
{
	PoolArray<i64, 512> pool = {};
	pool.Initialize(SAllocatorMalloc(), 0);

	SAssert(pool.BlockCount == 0);
	SAssert(pool.MemoryBlocks == 0);

	int n = pool.GetNext();
	SAssert(n == 0);
	*pool.At(n) = 100ll;
	int n2 = pool.GetNext();
	SAssert(n2 == 1);

	SAssert(pool.MemoryBlocks);
	SAssert(pool.BlockCount == 1);
	SAssert(pool.Head == 0);
	SAssert(pool.Tail == 2);
	SAssert(pool.TotalElements == 2);

	i64* np = pool.GetNextValue();
	SAssert(np);

	pool.Return(n2);

	SAssert(pool.MemoryBlocks);
	SAssert(pool.BlockCount == 1);
	SAssert(pool.Head == 1);
	SAssert(pool.Tail == 3);
	SAssert(pool.TotalElements == 2);

	pool.ReturnPtr(np);

	SAssert(pool.MemoryBlocks);
	SAssert(pool.BlockCount == 1);
	SAssert(pool.Head == 2);
	SAssert(pool.Tail == 3);
	SAssert(pool.TotalElements == 1);

	i64* atN = pool.At(n);
	SAssert(atN && *atN == 100);

	int n3 = pool.GetNext();
	SAssert(n3 == 2);
	*pool.At(n3) = 16;
	
	pool.Free();

	return 0;
}
