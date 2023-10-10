#pragma once

#include "StaticArray.h"
#include "ArrayList.h"

template<typename T, int Capacity>
struct SparseSet
{
	typedef u16 SparseSetSize_t;

	constant_var SparseSetSize_t EMPTY = 0xffff;

	template<typename T>
	struct SpareSetBucket
	{
		T Value;
		SparseSetSize_t Index;
	};


	SAllocator Allocator;
	ArrayList(SpareSetBucket<T>) Dense;
	StaticArray<SparseSetSize_t, Capacity> Sparse;

	void Initializer(SAllocator allocator)
	{
		Allocator = allocator;
		for (int i = 0; i < Capacity; ++i)
		{
			Sparse[i] = EMPTY;
		}
	}

	void Add(SparseSetSize_t id, const T* value)
	{
		SAssert(id < Capacity);

		SparseSetSize_t idx = ArrayListCount(Dense);
		Sparse.Data[id] = idx;

		SpareSetBucket bucket = {};
		bucket.Value = *value;
		bucket.Index = idx;

		ArrayListPush(Allocator, Dense, bucket);
	}

	SparseSetSize_t Remove(SparseSetSize_t id)
	{
		SAssert(id < Capacity);

		SparseSetSize_t idx = Sparse[id];
		Sparse[id] = EMPTY;

		SparseSetSize_t lastIdx = ArrayListCount(Dense) - 1;
		if (idx < lastIdx)
		{
			ArrayListPop(Dense, idx);
			SpareSetBucket* newId = Dense + idx;
			Sparse[newId->Index] = idx;
		}
		else if (idx == lastIdx)
		{
			ArrayListPopLast(Dense);
		}

		int shouldResize = ArrayListCapacity(Dense) >> 2;
		if (shouldResize > 8 && lastIdx < shouldResize)
		{
			ArrayListShrink(Allocator, Dense, shouldResize);
		}
		
		return idx;
	}

	bool Contains(SparseSetSize_t id)
	{
		SAssert(id < Capacity);
		return Sparse[id] != EMPTY;
	}

};
