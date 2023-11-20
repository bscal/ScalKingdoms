#pragma once

#include "Core.h"
#include "Memory.h"

#define ARRAY_LIST_DEFAULT_SIZE 8

// dynamic resizable array
#define ArrayList(T) T*

struct ArrayListHeader
{
	int Capacity;
	int Count;
};

#define ArrayListGetHeader(a) ((ArrayListHeader*)(a)-1)

#define ArrayListFree(_alloc, a) ((a) ? SFree(_alloc, ArrayListGetHeader(a)) : 0)
#define ArrayListPush(_alloc, a, value) (ArrayListMaybeGrow_Internal(_alloc, a, 1), (a)[ArrayListGetHeader(a)->Count++] = (value))
#define ArrayListCount(a) ((a) ? ArrayListGetHeader(a)->Count : 0)
#define ArrayListCapacity(a) ((a) ? ArrayListGetHeader(a)->Capacity : 0)
#define ArrayListAdd(_alloc, a, num) (ArrayListMaybeGrow_Internal(_alloc, a, num), ArrayListGetHeader(a)->Count += (num), &(a)[ArrayListGetHeader(a)->Count - (num)])
#define ArrayListLast(a) ((a)[ArrayListCount(a) - 1])
#define ArrayListPop(a, idx)  do { (a)[idx] = ArrayListLast(a);  --ArrayListGetHeader(a)->Count; } while (0)
#define ArrayListPopLast(a)  do { --ArrayListGetHeader(a)->Count; } while (0)
#define ArrayListPopLastN(a, num) do { ArrayListGetHeader(a)->Count -= (num); } while (0)
#define ArrayListClear(a) ((a) ? (ArrayListGetHeader(a)->Count = 0) : 0)
#define ArrayListReserve(_alloc, a, num) (ArrayListAdd(_alloc, a, num), ArrayListClear(a))
#define ArrayListPushAtIndex(_alloc, a, v, _index) \
	do {                                            \
		if ((_index) >= ArrayListCount(a))          \
			ArrayListPush(_alloc, a, v);            \
		else                                        \
			(a)[(_index)] = (v);                    \
	} while (0)

#define ArrayListShrink(_alloc, a, num) \
	do { \
    if (a && num < ArrayListCapacity(a) && num > ArrayListCount(a)) { \
        a = ArrayListShrink_Internal(_alloc, a, num, __FILE__, __FUNCTION__, __LINE__); \
	}     \
	} while (0)

inline void* 
ArrayListShrink_Internal(SAllocator allocator, void* arr, int num, int itemsize, const char* file, const char* function, int line)
{
	SAssert(IsAllocatorValid(allocator));
	int oldSize = ArrayListCapacity(arr) * itemsize + sizeof(ArrayListHeader);
	int newSize = num * itemsize + sizeof(ArrayListHeader);
	ArrayListHeader* p = (ArrayListHeader*)SRealloc(allocator, ArrayListGetHeader(arr), (size_t)newSize, (size_t)oldSize);
	SAssert(p);
	p[0].Capacity = num;
	SAssert(p[0].Count <= p[0].Capacity);
	return p + 1;
}

#define ArrayListMaybeGrow_Internal(_alloc, a, n) \
	(((!a) || (ArrayListGetHeader(a)->Count) + (n) >= (ArrayListGetHeader(a)->Capacity)) \
		? (*((void**)&(a)) = ArrayListGrow((_alloc), (a), (n), sizeof(*(a)), __FILE__, __FUNCTION__, __LINE__)) : 0)

inline void*
ArrayListGrow(SAllocator allocator, void* arr, int increment, int itemsize, const char* file, const char* function, int line)
{
	SAssert(IsAllocatorValid(allocator));
	ArrayListHeader header = (arr) ? *ArrayListGetHeader(arr) : ArrayListHeader{ };
	int oldCapacity = header.Capacity;
	int newCapacity = (oldCapacity < ARRAY_LIST_DEFAULT_SIZE) ? ARRAY_LIST_DEFAULT_SIZE : oldCapacity << 1;
	int minNeeded = header.Count + increment;
	int capacity = newCapacity > minNeeded ? newCapacity : minNeeded;
	header.Capacity = capacity;

	size_t oldSize = (arr) ? (size_t)itemsize * (size_t)oldCapacity + sizeof(ArrayListHeader) : 0;
	size_t size = (size_t)itemsize * (size_t)capacity + sizeof(ArrayListHeader);

	PushMemoryAdditionalInfo(file, function, line);
	ArrayListHeader* p = (ArrayListHeader*)SRealloc(allocator, arr ? ArrayListGetHeader(arr) : 0, size, oldSize);
	PopMemoryAdditionalInfo();

	if (p)
	{
		p[0] = header;
		return p + 1;
	}
	else
	{
		SError("Out of memory!");
		return nullptr;
	}
}

// dynamic allocated resizable void* array
struct SArray
{
	SAllocator Allocator;
	void* Memory;
	uint32_t Capacity;
	uint32_t Count;
	uint32_t Stride;
};

inline SArray ArrayCreate(SAllocator allocator, uint32_t capacity, uint32_t stride);
inline void ArrayFree(SArray* sArray);
inline void ArrayResize(SArray* sArray, uint32_t newCapacity);
inline void ArraySetCount(SArray* sArray, uint32_t count);
inline uint64_t GetArrayMemorySize(SArray* sArray);
inline void ArrayPush(SArray* sArray, const void* valuePtr);
inline void ArrayPop(SArray* sArray, void* dest);
inline void ArraySetAt(SArray* sArray, uint32_t index, const void* valuePtr);
inline void ArrayPopAt(SArray* sArray, uint32_t index, void* dest);
inline void* ArrayPeekAt(SArray* sArray, uint32_t index);
inline void ArrayClear(SArray* sArray);
inline void ArrayRemove(SArray* sArray);

inline SArray ArrayCreate(SAllocator allocator, uint32_t capacity, uint32_t stride)
{
	SAssert(IsAllocatorValid(allocator));
	SAssert(stride > 0);
	SArray sArray;
	sArray.Allocator = allocator;
	sArray.Memory = nullptr;
	sArray.Capacity = 0;
	sArray.Count = 0;
	sArray.Stride = stride;
	if (capacity > 0)
		ArrayResize(&sArray, capacity);
	return sArray;
}

inline void ArrayFree(SArray* sArray)
{
	SAssert(sArray);
	if (!sArray->Memory)
	{
		TraceLog(LOG_ERROR, "outSArray Memory is already freed!");
		return;
	}
	size_t size = (size_t)sArray->Capacity * sArray->Stride;
	SFree(sArray->Allocator, sArray->Memory);
}

inline void ArrayResize(SArray* sArray, uint32_t newCapacity)
{
	SAssert(sArray);
	SAssert(sArray->Stride > 0);

	if (newCapacity == 0)
		newCapacity = 8;

	if (newCapacity <= sArray->Capacity)
		return;

	size_t oldSize = (size_t)sArray->Capacity * sArray->Stride;
	size_t newSize = (size_t)newCapacity * sArray->Stride;
	sArray->Memory = SRealloc(sArray->Allocator, sArray->Memory, newSize, oldSize);
	sArray->Capacity = newCapacity;
}

inline void ArraySetCount(SArray* sArray, uint32_t count)
{
	SAssert(sArray);

	uint32_t initialCount = sArray->Count;
	if (initialCount >= count)
		return;

	ArrayResize(sArray, count);

	size_t start = (size_t)initialCount * sArray->Stride;
	size_t end = (size_t)sArray->Capacity * sArray->Stride;
	SAssert(end - start > 0);
	SAssert((uint8_t*)sArray->Memory + start <= (uint8_t*)sArray->Memory + end);
	SZero((uint8_t*)sArray->Memory + start, end - start);
	sArray->Count = count;
}

inline size_t GetArrayMemorySize(SArray* sArray)
{
	SAssert(sArray);
	return sArray->Capacity * sArray->Stride;
}

inline void ArrayPush(SArray* sArray, const void* valuePtr)
{
	SAssert(sArray);

	if (sArray->Count == sArray->Capacity)
	{
		ArrayResize(sArray, sArray->Capacity * 2);
	}

	char* dest = (char*)sArray->Memory;
	size_t offset = (size_t)sArray->Count * sArray->Stride;
	SCopy(dest + offset, valuePtr, sArray->Stride);
	++sArray->Count;
}

inline void ArrayPop(SArray* sArray, void* dest)
{
	SAssert(sArray);

	if (sArray->Count == 0) return;

	const char* src = (char*)(sArray->Memory);
	size_t offset = (size_t)(sArray->Count - 1) * sArray->Stride;
	SCopy(dest, src + offset, sArray->Stride);
	--sArray->Count;
}

inline void ArraySetAt(SArray* sArray, uint32_t index, const void* valuePtr)
{
	SAssert(sArray);
	SAssert(sArray->Memory);
	SAssert(sArray->Stride > 0);
	SAssert(index < sArray->Count);
	SAssert(index < sArray->Capacity);

	char* dest = (char*)(sArray->Memory);
	size_t offset = index * sArray->Stride;
	SCopy(dest + offset, valuePtr, sArray->Stride);
}

inline void ArrayPopAt(SArray* sArray, uint32_t index, void* dest)
{
	SAssert(sArray);
	SAssert(sArray->Memory);
	SAssert(sArray->Stride > 0);
	SAssert(index < sArray->Count);
	SAssert(index < sArray->Capacity);

	size_t offset = (size_t)index * sArray->Stride;
	char* popAtAddress = (char*)(sArray->Memory) + offset;
	SCopy(dest, popAtAddress, sArray->Stride);
	if (index != sArray->Count)
	{
		// Moves last element in array popped position
		size_t lastIndexOffset = (size_t)sArray->Count * sArray->Stride;
		char* lastIndexAddress = (char*)(sArray->Memory) + lastIndexOffset;
		SCopy(popAtAddress, lastIndexAddress, sArray->Stride);
	}
	--sArray->Count;
}

inline void* ArrayPeekAt(SArray* sArray, uint32_t index)
{
	SAssert(sArray);
	SAssert(sArray->Memory);
	SAssert(sArray->Stride > 0);
	SAssert(index < sArray->Count);
	SAssert(index < sArray->Capacity);

	size_t offset = (size_t)index * sArray->Stride;
	return ((char*)(sArray->Memory) + offset);
}

inline void ArrayClear(SArray* sArray)
{
	SAssert(sArray);
	sArray->Count = 0;
}

inline void ArrayRemove(SArray* sArray)
{
	SAssert(sArray);
	if (sArray->Count > 0)
	{
		--sArray->Count;
	}
}

inline bool ArrayRemoveAt(SArray* sArray, uint32_t index)
{
	SAssert(sArray);
	SAssert(sArray->Memory);
	SAssert(sArray->Stride > 0);
	SAssert(index < sArray->Count);
	SAssert(index < sArray->Capacity);

	size_t offset = (size_t)index * sArray->Stride;
	char* address = ((char*)(sArray->Memory)) + offset;
	bool shouldMoveLast = sArray->Count != 0 && index != (sArray->Count - 1);
	if (shouldMoveLast)
	{
		// Moves last element in array popped position
		uint64_t lastIndexOffset = (size_t)(sArray->Count - 1) * sArray->Stride;
		char* lastIndexAddress = ((char*)(sArray->Memory)) + lastIndexOffset;
		SCopy(address, lastIndexAddress, sArray->Stride);
	}
	--sArray->Count;
	return shouldMoveLast;
}

