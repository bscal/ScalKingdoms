#pragma once

#include "Core.h"
#include "Memory.h"

#include <stdint.h>

constexpr static uint32_t HASHMAP_NOT_FOUND = UINT32_MAX;
constexpr static uint32_t HASHMAP_ALREADY_CONTAINS = UINT32_MAX - 1;
constexpr static uint32_t HASHMAP_DEFAULT_CAPACITY = 2;
constexpr static uint32_t HASHMAP_DEFAULT_RESIZE = 2;
constexpr static float HASHMAP_LOAD_FACTOR = 0.85f;

#define HashMapGet(hashmap, hash, T) ((T*)HashMapGetPtr(hashmap, hash))
#define HashMapValuesIndex(hashmap, idx) (&((uint8_t*)(hashmap->Values))[hashmap->Stride * idx])

struct HashBucket;

struct HashMap
{
	HashBucket* Keys;
	void* Values;
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

uint32_t HashMapSet(HashMap* map, uint64_t hash, const void* value);

uint32_t HashMapSetEx(HashMap* map, uint64_t hash, const void* value, bool replace);

void* HashMapSetNew(HashMap* map, uint64_t hash);

uint32_t HashMapIndex(HashMap* map, uint64_t hash);

bool HashMapRemove(HashMap* map, uint64_t hash);

void HashMapForEach(HashMap* map, void(*Fn)(uint64_t, void*, void*), void* stackMemory);

_FORCE_INLINE_ void* 
HashMapGetPtr(HashMap* map, uint64_t hash)
{
	uint32_t idx = HashMapIndex(map, hash);
	return (idx == HASHMAP_NOT_FOUND) ? nullptr : HashMapValuesIndex(map, idx);
}


