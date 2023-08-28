#pragma once

#include "Core.h"
#include "Memory.h"

#include <stdint.h>

constexpr static uint32_t HASHMAP_NOT_FOUND = UINT32_MAX;
constexpr static uint32_t HASHMAP_DEFAULT_CAPACITY = 2;
constexpr static uint32_t HASHMAP_DEFAULT_RESIZE = 2;
constexpr static float HASHMAP_LOAD_FACTOR = 0.85f;

typedef void* (*HashMapAlloc)(size_t, size_t);
typedef void  (*HashMapFree)(void*, size_t);

#define HashMapGet(hashmap, hash, T) ((T*)HashMapGetPtr(hashmap, hash))
#define HashMapValuesOffset(hashmap, idx) ((hashmap).Values[idx * (hashmap).Stride])
#define HashMapValuesIndex(hashmap, idx, T) ((T*)HashMapValuesOffset(hashmap, idx))
#define HashMapReplace(hashmap, hash, value) \
		{ \
		SASSERT(value); \
		uint32_t idx = HashMapSet(hashmap, hash, value); \
		if (idx != HASHMAP_NOT_FOUND) \
			SCopy(HashMapValuesOffset(hashmap, idx), value, hashmap.Stride); \
		} \

struct HashBucket
{
	uint32_t Hash;
	uint32_t ProbeLength : 31;
	uint32_t IsUsed : 1;
};

struct HashMap
{
	HashBucket* Keys;
	uint8_t* Values;
	uint32_t Stride;
	uint32_t Capacity;
	uint32_t Count;
	uint32_t MaxCount;
	Allocator Alloc;
};

void HashMapInitialize(HashMap* map, uint32_t stride, uint32_t capacity, Allocator Alloc);

void HashMapReserve(HashMap* map, uint32_t capacity);

void HashMapClear(HashMap* map);

void HashMapDestroy(HashMap* map);

uint32_t HashMapSet(HashMap* map, uint32_t hash, const void* value);

void* HashMapSetNew(HashMap* map, uint32_t hash);

uint32_t HashMapIndex(HashMap* map, uint32_t hash);

bool HashMapRemove(HashMap* map, uint32_t hash);

void HashMapForEach(HashMap* map, void(*Fn(uint32_t, void*, void*)), void* stackMemory);

_FORCE_INLINE_ void* 
HashMapGetPtr(HashMap* map, uint32_t hash)
{
	uint32_t idx = HashMapIndex(map, hash);
	return (idx == HASHMAP_NOT_FOUND) ? nullptr : (void*)(&map->Values[idx]);
}
