#pragma once

#include "Core.h"
#include "Memory.h"

#include <stdint.h>

constexpr global_var u32 HASHMAPKV_NOT_FOUND = UINT32_MAX;

struct HashKVSlot;

typedef bool(*HashMapKVCompare)(const void*, const void*);

// Robin hood hashmap seperating Slots (probelength, tombstone), Keys, Values
struct HashMapKV
{
	HashKVSlot* Slots;
    void* Keys;
	void* Values;
	HashMapKVCompare CompareFunc;
	u32 KeyStride;
    u32 ValueStride;
    u32 Capacity;
	u32 Count;
	u32 MaxCount;
	Allocator Alloc;
};

void HashMapKVInitialize(HashMapKV* map, HashMapKVCompare compareFunc, 
	u32 keyStride, u32 valueStride, u32 capacity, Allocator alloc);

void HashMapKVReserve(HashMapKV* map, uint32_t capacity);

void HashMapKVClear(HashMapKV* map);

void HashMapKVFree(HashMapKV* map);

void HashMapKVSet(HashMapKV* map, const void* key, const void* value);

void HashMapKVReplace(HashMapKV* map, const void* key, const void* value);

void* HashMapKVSetZeroed(HashMapKV* map, const void* key);

u32 HashMapKVFind(HashMapKV* map, const void* key);

void* HashMapKVGet(HashMapKV* map, const void* key);

bool HashMapKVRemove(HashMapKV* map, const void* key);

void HashMapKVForEach(HashMapKV* map, void(*Fn)(void*, void*, void*), void* stackMemory);

#define HashMapKVMapGetT(hashmap, key, T) ((T*)HashMapGet(hashmap, key))
#define HashMapKVMapSetT(hashmap, key, T) ((T*)HashMapSetZeroed(hashmap, key))
