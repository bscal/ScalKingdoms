#pragma once

#include "Core.h"
#include "Memory.h"

#define ARRAY_LIST_DEFAULT_SIZE 8

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

#define ArrayListMaybeGrow_Internal(_alloc, a, n) \
    (((!a) || (ArrayListGetHeader(a)->Count) + (n) >= (ArrayListGetHeader(a)->Capacity)) \
        ? (*((void**)&(a)) = ArrayListGrow((_alloc), (a), (n), sizeof(*(a)))) : 0)

inline void* 
ArrayListGrow(Allocator alloc, void* arr, int increment, int itemsize)
{
    ArrayListHeader header = (arr) ? *ArrayListGetHeader(arr) : ArrayListHeader{ };
    int oldCapacity = header.Capacity;
    int newCapacity = (oldCapacity < ARRAY_LIST_DEFAULT_SIZE) ? ARRAY_LIST_DEFAULT_SIZE : oldCapacity << 1;
    int minNeeded = header.Count + increment;
    int capacity = newCapacity > minNeeded ? newCapacity : minNeeded;
    header.Capacity = capacity;

    size_t oldSize = (arr) ? (size_t)itemsize * (size_t)oldCapacity + sizeof(ArrayListHeader) : 0;
    size_t size = (size_t)itemsize * (size_t)capacity + sizeof(ArrayListHeader);
    ArrayListHeader* p = (ArrayListHeader*)SRealloc(alloc, arr ? ArrayListGetHeader(arr) : 0, size, oldSize);

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
