//
// Copyright 2018 Sepehr Taghdisian (septag@github). All rights reserved.
// License: https://github.com/septag/sx#license-bsd-2-clause
//
// Reference: https://en.wikipedia.org/wiki/Binary_heap
//
//  API:
//      sx_bheap_create     create binary heap object with a fixed capacity
//      sx_bheap_destroy    destroy binary heap object
//      sx_bheap_push_min   Push MIN value to binary heap, 'user' is a user-defined pointer
//      sx_bheap_push_max   Push MAX value to binary heap
//      sx_bheap_pop_min    Pop MIN value from binary heap
//      sx_bheap_pop_max    Pop MAX value from binary heap
//      sx_bheap_clear      Clears the binary heap
//      sx_bheap_empty      Returns true if binary heap is empty
//
//  NOTE: Only use either XXX_min or XXX_max APIs for a binary heap, don't mix them
//        use sx_bheap_push_max with sx_bheap_pop_max, or sx_bheap_push_min with sx_bheap_pop_max
//
#pragma once

#include "Memory.h"

typedef int(*CompareFunc)(void* v0, void* v1);

struct BHeapItem
{
    void* Key;
    void* User;
};

struct BHeap
{
    CompareFunc CompareFunc;
    BHeapItem* Items;
    int Count;
    int Capacity;
};

BHeap* BHeapCreate(SAllocator alloc, CompareFunc compareFunc, int capacity);
void BHeapDestroy(BHeap* bh, SAllocator alloc);

void BHeapPushMin(BHeap* bh, void* key, void* user);
BHeapItem BHeapPopMin(BHeap* bh);

void BHeapPushMax(BHeap* bh, void* key, void* user);
BHeapItem BHeapPopMax(BHeap* bh);

void BHeapClear(BHeap* bh);
bool BHeapEmpty(BHeap* bh);
