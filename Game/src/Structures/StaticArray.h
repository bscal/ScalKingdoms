#pragma once

#include "Core.h"

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

	constexpr _FORCE_INLINE_ const T& operator[](size_t idx) const
	{
		BoundsCheck(Data, idx, ElementCount);
		return Data[idx];
	}

	constexpr _FORCE_INLINE_ T& operator[](size_t idx)
	{
		BoundsCheck(Data, idx, ElementCount);
		return Data[idx];
	}

	constexpr _FORCE_INLINE_ size_t Count(void) const
	{
		return ElementCount;
	}

	constexpr _FORCE_INLINE_ size_t MemorySize() const
	{
		return ElementCount * sizeof(T);
	}
};

template<typename T>
struct DynamicArray
{
	T* Memory;
	size_t Count;

	void Initialize(size_t capacity, void*(*allocator)(size_t))
	{
		SAssert(capacity > 0);
		SAssert(allocator);
		Count = capacity
		Memory = (T*)allocator(Count * sizeof(T));
	}

	void Free(void(*free)(void*))
	{
		SAssert(free);
		if (Memory)
		{
			Memory = free(Memory);
			Memory = nullptr;
			Count = 0;
		}
	}

	void FromVarArgs(size_t capacity, void* (*allocator)(size_t), T values...)
	{
		Initialize(capacity, allocator);

		va_list ap;
		va_start(ap, values);

		for (uint32_t i = 0; i < count; ++i)
		{
			Memory[i] = va_arg(ap, T);
		}

		va_end(ap);
	}
};
