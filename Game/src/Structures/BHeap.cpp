#include "BHeap.h"

#include "Core.h"

static void sx__bheap_heapify_min(BHeap* bh, int index)
{
    int _2i = 2 * index;
    int _min = index;
    int count = bh->Count;
    BHeapItem min_item = bh->Items[index];

    while (_2i + 1 < count) {
        int left = _2i + 1;
        int right = _2i + 2;

        if (bh->Items[left].Key < bh->Items[_min].Key)
            _min = left;
        if (right < count && bh->Items[right].Key < bh->Items[_min].Key)
            _min = right;
        if (_min == index)
            break;

        bh->Items[index] = bh->Items[_min];
        bh->Items[_min] = min_item;
        index = _min;
        _2i = 2 * index;
    }
}

static void sx__bheap_heapify_max(BHeap* bh, int index)
{
    int _2i = 2 * index;
    int _max = index;
    int count = bh->Count;
    BHeapItem max_item = bh->Items[index];

    while (_2i + 1 < count) {
        int left = _2i + 1;
        int right = _2i + 2;

        if (bh->Items[left].Key > bh->Items[_max].Key)
            _max = left;
        if (right < count && bh->Items[right].Key > bh->Items[_max].Key)
            _max = right;
        if (_max == index)
            break;

        bh->Items[index] = bh->Items[_max];
        bh->Items[_max] = max_item;
        index = _max;
        _2i = 2 * index;
    }
}

BHeap* BHeapCreate(Allocator alloc, CompareFunc compareFunc, int capacity)
{
    size_t total_sz = sizeof(BHeap) + sizeof(BHeapItem) * capacity;
    BHeap* bh = (BHeap*)SMalloc(alloc, total_sz);

    SAssert(bh);

    bh->CompareFunc = compareFunc;
    bh->Items = (BHeapItem*)(bh + 1);
    bh->Count = 0;
    bh->Capacity = capacity;

    return bh;
}

void BHeapDestroy(BHeap* bh, Allocator alloc)
{
    SAssert(bh);

    SFree(alloc, bh);
}


void BHeapPushMin(BHeap* bh, void* key, void* user)
{
    SAssertMsg(bh->Count < bh->Capacity, "BinaryHeap's capacity exceeded");

    // Put the value at the bottom the tree and traverse up
    int index = bh->Count;
    bh->Items[index].Key = key;
    bh->Items[index].User = user;

    while (index > 0) {
        int parent_idx = (index - 1) >> 1;
        BHeapItem cur = bh->Items[index];
        BHeapItem parent = bh->Items[parent_idx];

        //if (cur.key < parent.key)
        int compare = bh->CompareFunc(cur.Key, parent.Key);
        if (compare < 0)
        {
            bh->Items[index] = parent;
            bh->Items[parent_idx] = cur;
        } else {
            break;
        }

        index = parent_idx;
    }
    ++bh->Count;
}

BHeapItem BHeapPopMin(BHeap* bh)
{
    SAssert(bh && bh->Count > 0);

    // Root is the value we want
    BHeapItem result = bh->Items[0];

    // put the last one on the root and heapify
    int last = bh->Count - 1;
    bh->Items[0] = bh->Items[last];
    bh->Count = last;

    sx__bheap_heapify_min(bh, 0);
    return result;
}

void BHeapPushMax(BHeap* bh, void* key, void* user)
{
    SAssertMsg(bh->Count < bh->Capacity, "BinaryHeap's capacity exceeded");

    // Put the value at the bottom the tree and traverse up
    int index = bh->Count;
    bh->Items[index].Key = key;
    bh->Items[index].User = user;

    while (index > 0) {
        int parent_idx = (index - 1) >> 1;
        BHeapItem cur = bh->Items[index];
        BHeapItem parent = bh->Items[parent_idx];

        //if (cur.key > parent.key)
        int compare = bh->CompareFunc(cur.Key, parent.Key);
        if (compare >= 0)
        {
            bh->Items[index] = parent;
            bh->Items[parent_idx] = cur;
        } else {
            break;
        }

        index = parent_idx;
    }
    ++bh->Count;
}

BHeapItem BHeapPopMax(BHeap* bh)
{
    SAssert(bh && bh->Count > 0);

    // Root is the value we want
    BHeapItem result = bh->Items[0];

    // put the last one on the root and heapify
    int last = bh->Count - 1;
    bh->Items[0] = bh->Items[last];
    bh->Count = last;

    sx__bheap_heapify_max(bh, 0);
    return result;
}


//BHeapItem* BHeapGet(BHeap* bh, void* key)
//{
//    int i, r = 0;													
//    BHeapItem* x = bh->Items;											
//        while (x) {
//            
//                i = __kb_getp_aux_##name(x, k, &r);							
//                if (i >= 0 && r == 0) return &__KB_KEY(key_t, x)[i];		
//                    if (x->is_internal == 0) return 0;							
//                        x = __KB_PTR(b, x)[i + 1];									
//        }																
//            return 0;
//}

void BHeapClear(BHeap* bh)
{
    bh->Count = 0;
}

bool BHeapEmpty(BHeap* bh)
{
    return bh->Count == 0;
}