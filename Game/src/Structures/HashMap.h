#pragma once

#include "Core.h"
#include "Memory.h"

constexpr internal_var u64 HASHMAPKV_NOT_FOUND = UINT64_MAX;

struct HashMapSlot
{
	u64 Key;
	u16 ProbeLength;
	bool IsUsed;
};

// Robin hood hashmap with 8 byte size key
struct HashMap
{
	SAllocator Alloc;
	HashMapSlot* Slots;
	void* Values;
    u32 ValueStride;
    u32 Capacity;
	u32 Count;
	u32 MaxCount;
};

void HashMapInitialize(HashMap* map, u32 valueStride, u32 capacity, SAllocator alloc);

void HashMapReserve(HashMap* map, uint32_t capacity);

void HashMapClear(HashMap* map);

void HashMapFree(HashMap* map);

void HashMapSet(HashMap* map, u64 key, const void* value);

void HashMapReplace(HashMap* map, u64 key, const void* value);

void* HashMapSetZeroed(HashMap* map, u64 key);

u64 HashMapFind(HashMap* map, u64 key);

void* HashMapGet(HashMap* map, u64 key);

bool HashMapRemove(HashMap* map, u64 key);

void HashMapForEach(HashMap* map, void(*Fn)(u64, void*, void*), void* stackMemory);

void* HashMapIndex(HashMap* map, u32 idx);
