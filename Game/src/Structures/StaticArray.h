#pragma once

#include "Core.h"
#include "Memory.h"

#include <stdarg.h>

#define BoundsCheck(data, idx, size) \
SAssert(idx < size); \
T* dataPtr = (T*)data; \
T* dataOffsetPtr = (T*)(data) + idx; \
SAssert(dataOffsetPtr >= (T*)dataPtr); \
SAssert(dataOffsetPtr < (T*)dataPtr + size); \

template<typename T, size_t ElementCount>
struct StaticArray
{
	T Data[ElementCount];

	constexpr _FORCE_INLINE_ T* At(size_t idx)
	{
		SAssert(idx < ElementCount);
		return Data + idx;
	}

	constexpr _FORCE_INLINE_ T AtCopy(size_t idx)
	{
		SAssert(idx < ElementCount);
		return Data[idx];
	}

	constexpr _FORCE_INLINE_ size_t Count()
	{
		return ElementCount;
	}

	constexpr _FORCE_INLINE_ size_t MemorySize()
	{
		return ElementCount * sizeof(T);
	}
};

template<typename T>
struct Buffer
{
	T* Data;
	int Capacity;
};

template<typename T>
void BufferResize(Buffer<T>* buffer, SAllocator allocator, int capacity)
{
	SAssert(buffer);
	SAssert(IsAllocatorValid(allocator));
	SAssert(capacity > 0);

	if (capacity == 0)
		return;

	if (buffer->Data)
	{
		BufferFree(buffer, allocator);
	}

	buffer->Data = SAlloc(allocator, capacity * sizeof(T));
	buffer->Capacity = capacity;
}

template<typename T>
void BufferFree(Buffer<T>* buffer, SAllocator allocator)
{
	SAssert(buffer);
	if (buffer->Data)
	{
		SFree(allocator, buffer);
	}
}

template<typename T>
T* BufferAt(Buffer<T>* buffer, size_t index)
{
	SAssert(buffer);
	SAssert(buffer->Data);
	SAssert(buffer->Capacity > 0);
	SAssert(index < buffer->Capacity);
	return buffer->Data + index;
}
