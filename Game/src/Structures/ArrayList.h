//
// Copyright 2018 Sepehr Taghdisian (septag@github). All rights reserved.
// License: https://github.com/septag/sx#license-bsd-2-clause
//
// array.h - v1.0 - Dynamic arrays
// Source: https://github.com/nothings/stb/blob/master/stretchy_buffer.h
//
// clang-format off
//    Unlike C++ vector<>, the sx_array has the same
//    semantics as an object that you manually malloc and realloc.
//    The pointer may relocate every time you add a new object
//    to it, so you:
//
//         1. can't take long-term pointers to elements of the array
//         2. have to return the pointer from functions which might expand it
//            (either as a return value or by storing it to a ptr-to-ptr)
//
//    Now you can do the following things with this array:
//
//         sx_array_free(sx_alloc* alloc, TYPE *a)          free the array
//         sx_array_count(TYPE *a)                          the number of elements in the array
//         sx_array_push(sx_alloc* alloc, TYPE *a, TYPE v)  adds v on the end of the array, a la push_back
//         sx_array_add(sx_alloc* alloc, TYPE *a, int n)    adds n uninitialized elements at end of array & returns pointer to first added
//         sx_array_last(TYPE *a)                           returns an lvalue of the last item in the array
//         sx_array_pop(TYPE *a, index)                     removes an element from the array and decreases array count
//         sx_array_pop_last(TYPE *a)                       removes last element from the array and decreases array count
//         a[n]                                             access the nth (counting from 0) element of the array
//         sx_array_clear(TYPE* a)                          resets the array count to zero, but does not resize memory
//         sx_array_reserve(sx_alloc* alloc, TYPE* a, int n) reserves N elements in array without incrementing the count
//         sx_array_push_byindex(alloc, TYPE* a, v, index)  receives index, if index is within the array, sets the array element to the value
//                                                          if not, pushes the element into the array
// Usage:
//       NOTE: include "allocator.h" before array.h to prevent warnings and errors
//       SomeStruct* SX_ARRAY arr = NULL;
//       while (something)
//       {
//          SomeStruct new_one;
//          new_one.whatever = whatever;
//          new_one.whatup   = whatup;
//          new_one.foobar   = barfoo;
//          sx_array_push(arr, new_one);
//       }
//
//     Note that these are all macros and many of them evaluate
//     their arguments more than once (except for 'v' in sx_array_push), so the arguments should
//     be side-effect-free.
//
//     Note that 'TYPE *a' in sx_array_push and sx_array_add must be lvalues
//     so that the library can overwrite the existing pointer if the object has to be reallocated.
//
#pragma once

#include "Core.h"
#include "Memory.h"

#define ARRAY_LIST_DEFAULT_SIZE 8

#define ArrayList(T) T*

#define ArrayListFree(_alloc, a) ((a) ? FreeAlign(_alloc, ArrayListRaw(a), 16) : 0)
#define ArrayListPush(_alloc, a, value) (ArrayListInternalMaybeGrow(_alloc, a, 1), (a)[ArrayListInternalCount(a)++] = (value))
#define ArrayListCount(a) ((a) ? ArrayListInternalCount(a) : 0)
#define ArrayListAdd(_alloc, a, num) (ArrayListInternalMaybeGrow(_alloc, a, num), ArrayListInternalCount(a) += (num), &(a)[ArrayListCount(a) - (num)])
#define ArrayListLast(a) ((a)[ArrayListCount(a) - 1])
#define ArrayListPop(a, idx)  do { (a)[idx] = ArrayListLast(a);  --ArrayListInternalCount(a); } while (0)
#define ArrayListPopLast(a)  do { --ArrayListInternalCount(a); } while (0)
#define ArrayListPopLastN(a, num) do { ArrayListInternalCount(a) -= (num); } while (0)
#define ArrayListClear(a) ((a) ? (ArrayListInternalCount(a) = 0) : 0)
#define ArrayListReserve(_alloc, a, num) (ArrayListAdd(_alloc, a, num), ArrayListClear(a))
#define ArrayListPushAtIndex(_alloc, a, v, _index) \
    do {                                            \
        if ((_index) >= ArrayListCount(a))          \
            ArrayListPush(_alloc, a, v);            \
        else                                        \
            (a)[(_index)] = (v);                    \
    } while (0)

////////////////////////////////////////////////////////////////////////////////////////////////////
// Internal
#define ArrayListRaw(a) ((int*)(a)-2)
#define ArrayListInternalCapacity(a) ArrayListRaw(a)[0]
#define ArrayListInternalCount(a) ArrayListRaw(a)[1]

#define ArrayListInternalNeedGrow(a, n) ((a) == 0 || ArrayListInternalCount(a) + (n) >= ArrayListInternalCapacity(a))
#define ArrayListInternalMaybeGrow(_alloc, a, n) (ArrayListInternalNeedGrow(a, (n)) ? ArrayListInternalGrow(_alloc, a, n) : 0)
#define ArrayListInternalGrow(_alloc, a, n)  (*((void**)&(a)) = ArrayListGrow((a), (n), sizeof(*(a)), (_alloc), __FILE__, __FUNCTION__, __LINE__))

inline void* 
ArrayListGrow(void* arr, int increment, int itemsize, Allocator alloc,
    const char* file, const char* func, int line)
{
    int oldCapacity = arr ? ArrayListInternalCapacity(arr) : 0;
    int newCapacity = (oldCapacity < ARRAY_LIST_DEFAULT_SIZE) ? oldCapacity << 1 : ARRAY_LIST_DEFAULT_SIZE;
    int minNeeded = ArrayListCount(arr) + increment;
    int capacity = newCapacity > minNeeded ? newCapacity : minNeeded;

    size_t oldSize = (size_t)itemsize * (size_t)oldCapacity + (sizeof(int) * 2);
    size_t size = (size_t)itemsize * (size_t)capacity + (sizeof(int) * 2);
    int* p = (int*)ReallocAlign(alloc, arr ? ArrayListRaw(arr) : 0, size, oldSize, 16);

    if (p)
    {
        p[0] = capacity;
        if (!arr)
            p[1] = 0;
        return p + 2;
    }
    else
    {
        SERR("Out of memory!");
        return nullptr;
    }
}
