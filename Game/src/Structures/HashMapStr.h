#pragma once

#include "Core.h"
#include "Memory.h"
#include "Lib/String.h"

constexpr internal_var u32 HASHMAPSTR_NOT_FOUND = UINT32_MAX;

struct HashStrSlot;

typedef bool(*HashMapKVCompare)(const void*, const void*);

struct HashStrSlot
{
	String String;
	u32 Hash;
	u16 ProbeLength;
	bool IsUsed;
};

// Robin hood hashmap seperating Slots (probelength, tombstone), Keys, Values
struct HashMapStr
{
	HashStrSlot* Buckets;
	void* Values;
	u32 ValueStride;
	u32 Capacity;
	u32 Count;
	u32 MaxCount;
	SAllocator Alloc;
};

void HashMapStrInitialize(HashMapStr* map, u32 valueStride, u32 capacity, SAllocator alloc);

void HashMapStrReserve(HashMapStr* map, uint32_t capacity);

void HashMapStrClear(HashMapStr* map);

void HashMapStrFree(HashMapStr* map);

void HashMapStrSet(HashMapStr* map, const String key, const void* value);

void HashMapStrReplace(HashMapStr* map, const String key, const void* value);

void* HashMapStrSetZeroed(HashMapStr* map, const String key);

u32 HashMapStrFind(HashMapStr* map, const String key);

void* HashMapStrGet(HashMapStr* map, const String key);

bool HashMapStrRemove(HashMapStr* map, const String key);

