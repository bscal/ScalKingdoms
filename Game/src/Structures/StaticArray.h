#pragma once

#include "Core/Core.h"
#include "Core/SMemory.h"

#include <stdarg.h>

#define BoundsCheck(data, idx, size) \
SASSERT(idx < size); \
T* dataPtr = (T*)data; \
T* dataOffsetPtr = (T*)(data) + idx; \
SASSERT(dataOffsetPtr >= (T*)dataPtr); \
SASSERT(dataOffsetPtr < (T*)dataPtr + size); \

template<typename T, size_t ElementCount>
struct SStaticArray
{
	T Data[ElementCount];

	_FORCE_INLINE_ const T& operator[](size_t idx) const
	{
		BoundsCheck(Data, idx, ElementCount);
		return Data[idx];
	}

	_FORCE_INLINE_ T& operator[](size_t idx)
	{
		BoundsCheck(Data, idx, ElementCount);
		return Data[idx];
	}

	_FORCE_INLINE_ void Fill(const T& value)
	{
		for (size_t i = 0; i < ElementCount; ++i)
		{
			SMemCopy(&Data[i], &value, sizeof(T));
		}
	}

	_FORCE_INLINE_ size_t Count(void) const
	{
		return ElementCount;
	}

	_FORCE_INLINE_ size_t MemorySize() const
	{
		return ElementCount * sizeof(T);
	}
};

template <typename T, size_t N>
using StaticArray = SStaticArray<T, N>;

template<typename T>
struct DynamicArray
{
	T* Memory;
	uint32_t Count;
	SAllocator Allocator;

	void Initialize(uint32_t capacity, SAllocator allocator)
	{
		SASSERT(Count == 0);

		Count = capacity;
		Allocator = allocator;
		Memory = (T*)SAlloc(Allocator, SizeOf(), MemoryTag::Arrays);
	}

	void FromVarArgs(uint32_t count, T values...)
	{
		Initialize(count, SAllocator::Game);

		va_list ap;
		va_start(ap, values);

		for (uint32_t i = 0; i < count; ++i)
		{
			Memory[i] = va_arg(ap, T);
		}

		va_end(ap);
	}

	_FORCE_INLINE_ T& operator[](uint32_t idx)
	{
		SASSERT(Memory);
		SASSERT(idx < Count);
		return Memory[idx];
	}

	_FORCE_INLINE_ size_t SizeOf() const { return Count * sizeof(T); }
};