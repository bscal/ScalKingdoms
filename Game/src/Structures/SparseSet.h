#pragma once

#include "Base.h"
#include "Memory.h"

template<typename T>
struct SparseSet
{
	typedef u16 SparseSetSize_t;

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

struct SparseArray
{
	SAllocator Allocator;
	u32* Sparse;
	u32* Dense;
	u32 Count;
	u32 SparseCapacity;
	u32 DenseCapacity;

	void Initialize(SAllocator allocator, u32 capacity)
	{
		SAssert(IsAllocatorValid(allocator));
		Allocator = allocator;
		SparseCapacity = capacity;
		Sparse = (u32*)SAlloc(Allocator, capacity * sizeof(u32));

		SAssert(Sparse);

		for (u32 i = 0; i < capacity; ++i)
		{
			Sparse[i] = UINT32_MAX;
		}
	}

	void Resize(u32 denseCapacity)
	{
		SAssert(IsAllocatorValid(Allocator));

		if (denseCapacity <= DenseCapacity)
			return;

		if (denseCapacity > SparseCapacity)
		{
			SError("SparseArray is full and cannot resize!");
			return;
		}

		Dense = (u32*)SRealloc(Allocator,
							   Dense,
							   denseCapacity * sizeof(u32),
							   DenseCapacity * sizeof(u32));
		DenseCapacity = denseCapacity;

		for (u32 i = Count; i < DenseCapacity; ++i)
		{
			Dense[i] = i;
		}
	}

	u32 Add(u32 id)
	{
		SAssert(Sparse);

		if (Count == DenseCapacity)
		{
			u32 newCap = (DenseCapacity) ? DenseCapacity * 2 : 1;
			Resize(newCap);
		}

		SAssert(Dense);
		u32 idx = Count;
		++Count;

		Dense[idx] = id;
		Sparse[id] = idx;
		return idx;
	}

	u32 Remove(u32 id)
	{
		SAssert(Sparse);
		SAssert(Dense);
		SAssert(id < SparseCapacity);

		if (Count > 0 && Contains(id))
		{
			u32 idx = Sparse[id];
			u32 lastIdx = Count - 1;
			u32 lastId = Dense[lastIdx];

			Sparse[lastId] = UINT32_MAX;
			Dense[idx] = lastId;
			Dense[lastIdx] = id;

			--Count;

			return idx;
		}
		return UINT32_MAX;
	}

	int Get(u32 id)
	{
		SAssert(Sparse);
		SAssert(Dense);
		SAssert(id < SparseCapacity);
		if (Contains(id))
			return Dense[Sparse[id]];
		else
			return UINT32_MAX;
	}

	bool Contains(u32 id)
	{
		SAssert(Sparse);
		SAssert(Dense);
		SAssert(id < SparseCapacity);
		return Sparse[id] != UINT32_MAX;
	}
};

inline void TestSpareSet()
{
	SparseSet<u8> TestSet = {};
	TestSet.Initialize(SAllocatorMalloc(), 64);

	u8 v = 16;
	u16 r = TestSet.Add(&v);

	u8 vv = 32;
	u16 rr = TestSet.Add(&vv);

	SAssert(TestSet.Count == 2);
	SAssert(TestSet.Dense[0].Value == v);
	SAssert(r == 0);
	SAssert(rr == 1);

	u8 vvv = 64;
	u16 rrr = TestSet.Add(&vvv);

	TestSet.Remove(rr);

	SAssert(rrr == 2);

	SAssert(*TestSet.Get(r) == 16);
	SAssert(!TestSet.Get(rr));
	SAssert(TestSet.Count == 2);
}
