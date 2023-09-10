#pragma once

#include "Core.h"
#include "Memory.h"

constexpr global_var u32 HASHMAPKV_NOT_FOUND = UINT32_MAX;

struct HashMapSlot
{
	u8 ProbeLength;
	bool IsUsed;
};

typedef bool(*HashMapCompare)(const void*, const void*);

// Robin hood hashmap seperating Slots (probelength, tombstone), Keys, Values
struct HashMap
{
	HashMapSlot* Slots;
    void* Keys;
	void* Values;
	HashMapCompare CompareFunc;
	u32 KeyStride;
    u32 ValueStride;
    u32 Capacity;
	u32 Count;
	u32 MaxCount;
	Allocator Alloc;
};

void HashMapInitialize(HashMap* map, HashMapCompare compareFunc, 
	u32 keyStride, u32 valueStride, u32 capacity, Allocator alloc);

void HashMapReserve(HashMap* map, uint32_t capacity);

void HashMapClear(HashMap* map);

void HashMapFree(HashMap* map);

void HashMapSet(HashMap* map, const void* key, const void* value);

void HashMapReplace(HashMap* map, const void* key, const void* value);

void* HashMapSetZeroed(HashMap* map, const void* key);

u32 HashMapFind(HashMap* map, const void* key);

void* HashMapGet(HashMap* map, const void* key);

bool HashMapRemove(HashMap* map, const void* key);

void HashMapForEach(HashMap* map, void(*Fn)(void*, void*, void*), void* stackMemory);

void* HashMapIndex(HashMap* map, u32 idx);
