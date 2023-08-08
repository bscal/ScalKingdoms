#pragma once

#include "Core.h"

#include <stdint.h>

constexpr static uint32_t HASHMAP_NOT_FOUND = UINT32_MAX;
constexpr static uint32_t HASHMAP_DEFAULT_CAPACITY = 2;
constexpr static uint32_t HASHMAP_DEFAULT_RESIZE = 2;
constexpr static float HASHMAP_LOAD_FACTOR = 0.80f;

typedef void* (*HashMapAlloc)(size_t, size_t);
typedef void(*HashMapFree)(void*, size_t);

#define HashMapGet(hashmap, hash, T) ((T*)hashmap.Get(hash))
#define HashMapValuesIndex(hashmap, idx, T) ((T)hashmap->Values[idx * hashmap->Stride])

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
	HashMapAlloc Allocate;
	HashMapFree Free;

	void Initialize(uint32_t stride, uint32_t capacity, HashMapAlloc allocate, HashMapFree free);

	void Reserve(uint32_t capacity);
	
	void Destroy();

	uint32_t Put(uint32_t hash, void* value);

	uint32_t Index(uint32_t hash);

	bool Remove(uint64_t hash);

	void ForEach(void(*Fn(uint32_t, void*, void*)), void* stackMemory);

	inline uint8_t* Get(uint32_t hash)
	{
		uint32_t idx = Index(hash);
		return (idx == HASHMAP_NOT_FOUND) ? nullptr : &Values[idx];
	}
};
