#pragma once

#include "Allocator.h"

typedef u16 SparseSetSize_t;

template<typename T>
struct SparseSet
{
	constant_var SparseSetSize_t EMPTY = 0xffff;

	template<typename T>
	struct SparseSetBucket
	{
		T Value;
		SparseSetSize_t Id;
	};

	SAllocator Allocator;
	SparseSetSize_t* Sparse;
	SparseSetBucket<T>* Dense;
	SparseSetSize_t SparseCapacity;
	SparseSetSize_t DenseCapacity;
	SparseSetSize_t Count;

	void Initialize(SAllocator allocator, int capacity)
	{
		SAssert(IsAllocatorValid(allocator));
		Allocator = allocator;
		SparseCapacity = (SparseSetSize_t)capacity;
		Sparse = (SparseSetSize_t*)SAlloc(Allocator, capacity * sizeof(SparseSetSize_t));

		SAssert(Sparse);

		for (int i = 0; i < capacity; ++i)
		{
			Sparse[i] = EMPTY;
		}
	}

	void Resize(int denseCapacity)
	{
		Dense = (SparseSetBucket<T>*)SRealloc(Allocator,
											  Dense, 
											  denseCapacity * sizeof(SparseSetBucket<T>),
											  DenseCapacity * sizeof(SparseSetBucket<T>));
		DenseCapacity = (SparseSetSize_t)denseCapacity;

		for (SparseSetSize_t i = Count; i < DenseCapacity; ++i)
		{
			Dense[i].Id = i;
		}
	}

	SparseSetSize_t Add(T* value)
	{
		SAssert(IsAllocatorValid(Allocator));
		SAssert(Sparse);

		if (Count < SparseCapacity)
		{
			if (Count == DenseCapacity)
			{
				int newCap = (DenseCapacity) ? DenseCapacity * 2 : 1;
				Resize(newCap);
			}

			SAssert(Dense);

			SparseSetSize_t idx = Count;
			++Count;

			Dense[idx].Value = *value;
			Sparse[Dense[idx].Id] = idx;

			return Dense[idx].Id;
		}
		else
		{
			SError("SparseSet is full!");
			return EMPTY;
		}
	}

	void Remove(SparseSetSize_t id)
	{
		SAssert(IsAllocatorValid(Allocator));
		SAssert(Sparse);
		SAssert(Dense);
		SAssert(id < SparseCapacity);

		SparseSetSize_t idx = Sparse[id];
		SparseSetBucket<T>* lastBucket = Dense + Count - 1;

		Sparse[lastBucket->Id] = EMPTY;
		Dense[idx] = *lastBucket;
		Dense[Count - 1].Id = id;
		
		--Count;
	}

	T* Get(SparseSetSize_t id)
	{
		SAssert(Sparse);
		SAssert(Dense);
		SAssert(id < SparseCapacity);

		SparseSetSize_t idx = Sparse[id];

		if (idx != EMPTY && Dense[idx].Id == id)
			return &Dense[idx].Value;
		else
			return nullptr;
	}
};

inline void TestSpareSet()
{
	SparseSet<u8> TestSet = {};
	TestSet.Initialize(SAllocatorMalloc(), 64);

	u8 v = 16;
	SparseSetSize_t r = TestSet.Add(&v);

	u8 vv = 32;
	SparseSetSize_t rr = TestSet.Add(&vv);

	SAssert(TestSet.Count == 2);
	SAssert(TestSet.Dense[0].Value == v);
	SAssert(r == 0);
	SAssert(rr == 1);

	u8 vvv = 64;
	SparseSetSize_t rrr = TestSet.Add(&vvv);

	TestSet.Remove(rr);

	SAssert(rrr == 2);

	SAssert(*TestSet.Get(r) == 16);
	SAssert(!TestSet.Get(rr));
	SAssert(TestSet.Count == 2);
}
