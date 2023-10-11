#pragma once

#include "Allocator.h"
#include "ArrayList.h"

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
	ArrayList(SparseSetSize_t) Sparse;
	ArrayList(SparseSetBucket<T>) Dense;
	SparseSetSize_t Count;

	void Initialize(SAllocator allocator, int capacity)
	{
		SAssert(IsAllocatorValid(allocator));
		Allocator = allocator;
		ArrayListReserve(Allocator, Sparse, capacity);

		SAssert(Sparse);
		ArrayListHeader* header = ArrayListGetHeader(Sparse);
		header->Count = header->Capacity;

		for (int i = 0; i < header->Capacity; ++i)
		{
			Sparse[i] = EMPTY;
		}
	}

	void Resize(int denseCapacity)
	{
		ArrayListReserve(Allocator, Dense, denseCapacity);

		for (SparseSetSize_t i = (SparseSetSize_t)ArrayListCount(Dense); i < (SparseSetSize_t)ArrayListCapacity(Dense); ++i)
		{
			SparseSetBucket<T> tmp = {};
			tmp.Id = i;
			ArrayListPush(Allocator, Dense, tmp);
		}
	}

	SparseSetSize_t Add(T* value)
	{
		SAssert(IsAllocatorValid(Allocator));
		SAssert(Sparse);

		if (Count < ArrayListCapacity(Sparse))
		{
			if (Count == ArrayListCapacity(Dense))
			{
				int newCap = (ArrayListCapacity(Dense)) ? ArrayListCapacity(Dense) * 2 : 1;
				Resize(newCap);
			}

			SAssert(Dense);

			SparseSetSize_t idx = Count;
			++Count;

			Dense[idx].Value = *value;
			Sparse[Dense[idx].Id] = idx;

			return Dense[idx].Id;
		}
		return EMPTY;
	}

	void Remove(SparseSetSize_t id)
	{
		SAssert(IsAllocatorValid(Allocator));
		SAssert(Sparse);
		SAssert(Dense);
		SAssert(id < (SparseSetSize_t)ArrayListCapacity(Sparse));

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
		SAssert(id < (SparseSetSize_t)ArrayListCapacity(Sparse));

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
