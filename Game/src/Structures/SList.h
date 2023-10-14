#pragma once

#include "Core.h"
#include "Memory.h"

constant_var uint32_t SLIST_NO_FOUND = UINT32_MAX;

template<typename T>
struct SList
{
	SAllocator Alloc;
	T* Memory;
	uint32_t Capacity;
	uint32_t Count;

	void Free();
	void Reserve(SAllocator allocator, uint32_t capacity); // allocated memory based on capacity
	void EnsureSize(SAllocator allocator, uint32_t ensuredCount); // ensures capacity and count elements
	void Resize(); // Resizes the array, if size = 0, then size will be 1

	void Push(const T* valueSrc); // Checks resize, inserts and end of array
	T* PushNew(); // Checks resize, default constructs next element, and returns pointer
	void PushAt(uint32_t index, const T* valueSrc);
	void PushAtFast(uint32_t index, const T* valueSrc); // Checks resize, inserts at index, moving old index to end
	bool PushUnique(const T* valueSrc);
	bool PushAtUnique(uint32_t index, const T* valueSrc);
	bool PushAtFastUnique(uint32_t index, const T* valueSrc);
	void Pop(T* valueDest); // Pops end of array
	void PopAt(uint32_t index, T* valueDest);
	void PopAtFast(uint32_t index, T* valueDest); // pops index, moving last element to index
	void Remove(); // Same as pop, but does not do a copy
	bool RemoveAt(uint32_t index);
	bool RemoveAtFast(uint32_t index);

	bool Contains(const T* value) const;
	uint32_t Find(const T* value) const;
	void Copy(T* arr, uint32_t count);
	
	_FORCE_INLINE_ void Clear();

	_FORCE_INLINE_ T* At(size_t idx)
	{
		SAssert(idx < Count);
		SAssert(Memory);
		return Memory + idx;
	}

	_FORCE_INLINE_ T AtCopy(size_t idx)
	{
		SAssert(idx < Count);
		SAssert(Memory);
		return Memory[idx];
	}

	_FORCE_INLINE_ size_t MemUsed() const { return Capacity * sizeof(T); }

	_FORCE_INLINE_ uint32_t LastIndex() const
	{
		return (Count == 0) ? 0 : (Count - 1);
	}

	_FORCE_INLINE_ T* Last() const
	{
		SAssert(Memory);
		return &Memory[LastIndex()];
	}
};

// ********************

template<typename T>
void SList<T>::Free()
{
	SAssert(Memory);
	SAssert(IsAllocatorValid(Alloc));
	SFree(Alloc, Memory);
	Memory = nullptr;
	Capacity = 0;
	Count = 0;
}

template<typename T>
void SList<T>::Reserve(SAllocator allocator, uint32_t newCapacity)
{
	SAssert(IsAllocatorValid(allocator));
	SAssert(newCapacity > 0);

	if (newCapacity <= Capacity)
		return;

	size_t newSize = newCapacity * sizeof(T);
	size_t oldSize = Capacity * sizeof(T);
	Memory = (T*)SRealloc(allocator, Memory, newSize, oldSize);
	Alloc = allocator;
	Capacity = newCapacity;
	SAssert(Memory);
	SAssert(Count <= newCapacity);
}

template<typename T>
void SList<T>::EnsureSize(SAllocator allocator, uint32_t ensuredCount)
{
	if (ensuredCount <= Count)
		return;

	Reserve(allocator, ensuredCount);

	for (uint32_t i = Count; i < ensuredCount; ++i)
	{
		Memory[i] = T{};
	}
	Count = ensuredCount;

	SAssert(ensuredCount <= Count);
	SAssert(ensuredCount <= Capacity);
	SAssert(Count <= Capacity);
}

template<typename T>
_FORCE_INLINE_ void SList<T>::Resize()
{
	u32 newCapacity = (Capacity == 0) ? 1 : Capacity * 2;
	Reserve(Alloc, newCapacity);
}

template<typename T>
void SList<T>::Push(const T* valueSrc)
{
	SAssert(valueSrc);
	SAssert(Count <= Capacity);
	if (Count == Capacity)
	{
		Resize();
	}
	Memory[Count] = *valueSrc;
	++Count;
}

template<typename T>
T* SList<T>::PushNew()
{
	SAssert(Count <= Capacity);
	if (Count == Capacity)
	{
		Resize();
	}
	
	T* pos = Memory + Count;
	++Count;
	return pos;
}

template<typename T>
void SList<T>::PushAt(uint32_t index, const T* valueSrc)
{
	SAssert(index < Count);
	SAssert(valueSrc);

	if (Count == Capacity)
	{
		Resize();
	}

	if (index != LastIndex())
	{
		T* dst = Memory + index + 1;
		T* src = Memory + index;
		size_t sizeTillEnd = (Count - index) * sizeof(T);
		memmove(dst, src, sizeTillEnd);
	}
	Memory[index] = *valueSrc;
	++Count;
}

template<typename T>
void SList<T>::PushAtFast(uint32_t index, const T* valueSrc)
{
	SAssert(index < Count);
	SAssert(valueSrc);
	SAssert(Count <= Capacity);
	if (Count == Capacity)
	{
		Resize();
	}
	if (index != LastIndex())
	{
		memmove(Memory + Count, Memory + index, sizeof(T));
	}
	Memory[index] = *valueSrc;
	++Count;
}

template<typename T>
bool SList<T>::PushUnique(const T* valueSrc)
{
	if (Contains(valueSrc)) return false;
	Push(valueSrc);
	return true;
}

template<typename T>
bool SList<T>::PushAtUnique(uint32_t index, const T* valueSrc)
{
	if (Contains(valueSrc)) return false;
	PushAt(index, valueSrc);
	return true;
}

template<typename T>
bool SList<T>::PushAtFastUnique(uint32_t index, const T* valueSrc)
{
	if (Contains(valueSrc)) return false;
	PushAtFast(index, valueSrc);
	return true;
}

template<typename T>
void SList<T>::Pop(T* valueDest)
{
	SAssert(Memory);
	SAssert(Count > 0);
	SAssert(Count <= Capacity);
	SCopy(valueDest, Memory + LastIndex(), sizeof(T));
	--Count;
}

template<typename T>
void SList<T>::PopAt(uint32_t index, T* valueDest)
{
	SAssert(Memory);
	SAssert(Count > 0);
	SAssert(index < Count);
	SAssert(Count <= Capacity);
	memcpy(valueDest, Memory + index, sizeof(T));
	if (index != LastIndex())
	{
		T* dst = Memory + index;
		T* src = Memory + index + 1;
		size_t size = (size_t)(LastIndex() - index) * sizeof(T);
		memmove(dst, src, size);
	}
	--Count;
}

template<typename T>
void SList<T>::PopAtFast(uint32_t index, T* valueDest)
{
	SAssert(Memory);
	SAssert(Count > 0);
	SAssert(index < Count);
	SAssert(Count <= Capacity);
	memcpy(valueDest, Memory + index, sizeof(T));
	if (index != LastIndex())
	{
		memcpy(Memory + index, Memory + LastIndex(), sizeof(T));
	}
	--Count;
}

template<typename T>
void SList<T>::Remove()
{
	SAssert(Memory);
	SAssert(Count > 0);
	--Count;
}

template<typename T>
bool SList<T>::RemoveAt(uint32_t index)
{
	SAssert(Memory);
	SAssert(Count > 0);
	--Count;
	if (index != Count)
	{
		size_t dstOffset = index * sizeof(T);
		size_t srcOffset = ((size_t)(index) + 1) * sizeof(T);
		size_t sizeTillEnd = ((size_t)(Count) -(size_t)(index)) * sizeof(T);
		char* mem = (char*)Memory;
		memmove(mem + dstOffset, mem + srcOffset, sizeTillEnd);
		return true;
	}
	return false;
}

template<typename T>
bool SList<T>::RemoveAtFast(uint32_t index)
{
	SAssert(Memory);
	SAssert(Count > 0);
	SAssert(index < Count);

	bool wasLastSwapped = false;

	if (index != LastIndex())
	{
		SMemCopy(Memory + index, Memory + LastIndex(), sizeof(T));
		wasLastSwapped = true;
	}

	--Count;
	return wasLastSwapped;
}

template<typename T>
void SList<T>::Copy(T* arr, uint32_t count)
{
	SAssert(arr);
	SAssert(count > 0);

	EnsureSize(count);
	memcpy(Memory, arr, count * sizeof(T));
}

template<typename T>
bool SList<T>::Contains(const T* value) const
{
	SAssert(value);
	for (uint32_t i = 0; i < Count; ++i)
	{
		if (Memory[i] == *value)
			return true;
	}
	return false;
}

template<typename T>
uint32_t SList<T>::Find(const T* value) const
{
	SAssert(value);
	for (uint32_t i = 0; i < Count; ++i)
	{
		if (Memory[i] == *value)
			return i;
	}
	return SLIST_NO_FOUND;
}

template<typename T>
void SList<T>::Clear()
{
	Count = 0;
}
