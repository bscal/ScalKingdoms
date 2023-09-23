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
