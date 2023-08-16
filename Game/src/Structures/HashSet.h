#pragma once

#include "Core.h"
#include "Memory.h"

constexpr static uint32_t HASHSET_NOT_FOUND = UINT32_MAX;
constexpr static uint32_t HASHSET_DEFAULT_CAPACITY = 2;
constexpr static uint32_t HASHSET_DEFAULT_RESIZE = 2;
constexpr static float HASHSET_LOAD_FACTOR = 0.85f;

struct HashSetBucket
{
	uint32_t Hash;
	uint32_t ProbeLength : 31;
	uint32_t IsUsed : 1;
};

struct HashSet
{
	HashSetBucket* Keys;
	uint32_t Capacity;
	uint32_t Count;
	Allocator Alloc;

	void Initialize(uint32_t capacity, Allocator allocator);

	void Reserve(uint32_t capacity);

	void Clear();

	void Destroy();

	bool Put(uint32_t hash);

	bool Contains(uint32_t hash);

	bool Remove(uint32_t hash);
};
