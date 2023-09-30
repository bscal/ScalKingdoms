#pragma once

#include "Core.h"
#include "Memory.h"

constexpr static uint32_t HASHSET_NOT_FOUND = UINT32_MAX;
constexpr static uint32_t HASHSET_DEFAULT_CAPACITY = 2;
constexpr static uint32_t HASHSET_DEFAULT_RESIZE = 2;
constexpr static float HASHSET_LOAD_FACTOR = 0.85f;

struct HashSetBucket
{
	uint64_t Hash;
	uint32_t ProbeLength;
	bool IsUsed;
};

struct HashSet
{
	HashSetBucket* Keys;
	uint32_t Capacity;
	uint32_t Count;
	SAllocator Alloc;
};

void HashSetInitialize(HashSet* set, uint32_t capacity, SAllocator SAllocator);

void HashSetReserve(HashSet* set, uint32_t capacity);

void HashSetClear(HashSet* set);

void HashSetDestroy(HashSet* set);

bool HashSetSet(HashSet* set, uint64_t hash);

bool HashSetContains(HashSet* set, uint64_t hash);

bool HashSetRemove(HashSet* set, uint64_t hash);
