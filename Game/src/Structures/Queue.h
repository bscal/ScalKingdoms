#pragma once

#include "Core.h"
#include "Memory.h"

template<typename T>
struct Queue
{
	T* Memory;
	int First;
	int Last;
	int Count;
	int Capacity;
	Allocator Allocator;

	void Init(enum Allocator allocator, int capacity)
	{
		SAssert(IsAllocatorValid(allocator));
		SAssert(capacity > 0)

		Allocator = allocator;
		Capacity = capacity;
		Memory = (T*)SMalloc(this->Allocator, Capacity * sizeof(T));

		SAssert(Memory);
	}

	void Free()
	{
		SFree(Allocator, Memory);
	}

	_FORCE_INLINE_ bool IsEmpty() { return Count == 0; }
	_FORCE_INLINE_ bool IsFull() { return Count == Capacity; }
	_FORCE_INLINE_ int LastIndex() { return (Last - 1) & (Capacity - 1); }
	_FORCE_INLINE_ T* At(int i)
	{
		SAssert(Memory);
		SAssert(i < Count);
		int idx = (First + i) % Capacity;
		SAssert(Memory + idx >= Memory);
		SAssert(Memory + idx < Memory + Capacity);
		return &Memory[idx];
	}

	bool PushLast(const T* value)
	{
		SAssert(value);
		SAssert(Memory);
		SAssert(IsAllocatorValid(Allocator));

		if (IsFull())
			return false;

		Memory[Last] = *value;
		++Count;
		
		++Last;
		if (Last == Capacity)
			Last = 0;

		return true;
	}

	T* PeekFirst()
	{
		if (IsEmpty())
			return nullptr;
		else
		{
			SAssert(Memory);
			return &Memory[First];
		}
	}

	T* PeekLast()
	{
		if (IsEmpty())
			return nullptr;
		else
		{
			SAssert(Memory);
			SAssert(Capacity > 0);
			return &Memory[LastIndex()];
		}
	}

	void PopFirst()
	{
		if (!IsEmpty())
		{
			--Count;
			++First;
			if (First == Capacity)
				First = 0;
		}
	}

};