#pragma once

#include "Core.h"
#include "Memory.h"

constexpr global_var u32 HASHTABLE_NOT_FOUND = UINT32_MAX;

#ifndef HashTableHasher
#define HashTableHasher(data, len, cap) (zpl_fnv32a((data), (len)) & ((cap) - 1))
#endif

template<typename K, typename V>
struct HashTableBucket
{
	K Key;
	u32 ProbeLength : 31;
	u32 IsUsed : 1;
	V Value;
};

template<typename K, typename V>
struct HashTable
{
	HashTableBucket<K, V>* Buckets;
	u32 Capacity;
	u32 Count;
	u32 MaxCount;
	float LoadFactor;
	Allocator Alloc;
};

template<typename K, typename V>
void HashTableInitialize(HashTable<K, V>* map, uint32_t capacity, float loadFactor, Allocator Alloc)
{
	SASSERT(map);
	SASSERT(IsAllocatorValid(alloc));

	map->Alloc = alloc;
	map->LoadFactor = loadFactor;
	if (capacity > 0)
		HashMapReserve(map, capacity);
}

template<typename K, typename V>
void HashTableReserve(HashTable<K, V>* map, uint32_t capacity)
{
	SASSERT(IsAllocatorValid(map->Alloc));

	if (capacity == 0)
		capacity = 2;

	if (capacity <= map->Capacity)
		return;

	if (!IsPowerOf2_32(capacity))
		capacity = AlignPowTwo32(capacity);

	if (map->Keys)
	{
		HashTable<K, V> tmpMap = {};
		tmpMap.Alloc = map->Alloc;
		tmpMap.LoadFactor = map->LoadFactor;
		HashTableReserve(&tmpMap, capacity);

		for (uint32_t i = 0; i < map->Capacity; ++i)
		{
			if (map->Keys[i].IsUsed)
			{
				HashTableSet(&tmpMap, map->Keys[i].Hash, HashMapValuesIndex(map, i));
			}
		}

		GameFree(map->Alloc, map->Buckets);

		SASSERT(map->Count == tmpMap.Count);
		*map = tmpMap;
	}
	else
	{
		map->Capacity = capacity;

		size_t size = sizeof(HashTableBucket<K, V>) * map->Capacity;

		map->Buckets = (HashTableBucket*)GameMalloc(map->Alloc, size);

		memset(map->Buckets, 0, size);

		if (map->LoadFactor <= 0.0f)
			map->LoadFactor = 0.85f;

		map->MaxCount = (uint32_t)((float)map->Capacity * map->LoadFactor);
	}
}

template<typename K, typename V>
void HashTableClear(HashTable<K, V>* map)
{
	SClear(map->Buckets, sizeof(HashTableBucket<K, V>) * map->Capacity);
	map->Count = 0;
}

template<typename K, typename V>
void HashTableDestroy(HashTable<K, V>* map)
{
	GameFree(map->Alloc, map->Buckets);
	map->Capacity = 0;
	map->Count = 0;
}

template<typename K, typename V>
uint32_t HashTableSet(HashTable<K, V>* map, K* key)
{
	if (map->Count >= map->MaxCount)
	{
		HashTableReserve(map, map->Capacity * 2);
	}

	HashTableBucket<K, V> swapBucket = {};
	swapBucket.Key = *key;

	uint32_t insertedIndex = HASHTABLE_NOT_FOUND;
	uint32_t probeLength = 0;
	uint32_t idx = HashTableHasher(key, sizeof(K), map->Capacity);
	while (true)
	{
		HashTableBucket<K, V>* bucket = &map->Buckets[idx];
		if (!bucket->IsUsed) // Bucket is not used
		{
			*bucket = swapBucket;
			bucket->ProbeLength = probeLength;
			bucket->IsUsed = 1;
			++map->Count;

			if (insertedIndex == HASHTABLE_NOT_FOUND)
				insertedIndex = idx;

			break;
		}
		else
		{
			if (*bucket->Key == *key)
				return idx;

			if (probeLength > bucket->ProbeLength)
			{
				if (insertedIndex == HASHTABLE_NOT_FOUND)
					insertedIndex = idx;

				Swap(bucket->ProbeLength, probeLength, uint32_t);
				Swap(bucket, swapBucket, auto);
			}

			++probeLength;
			++idx;
			if (idx == map->Capacity)
				idx = 0;
		}
	}

	return insertedIndex;
}

template<typename K, typename V>
K* HashTableFind(HashTable<K, V>* map, K* key)
{
	if (!map->Buckets || map->Count == 0)
		return nullptr;

	uint32_t probeLength = 0;
	uint32_t idx = HashTableHasher(key, sizeof(K), map->Capacity);
	while (true)
	{
		HashTableBucket<K, V>* bucket = &map->Buckets[idx];
		if (!bucket.IsUsed || probeLength > bucket.ProbeLength)
			return nullptr;
		else if (*key == *bucket->Key)
			return &bucket->Value;
		else
		{
			++probeLength;
			++idx;
			if (idx == map->Capacity)
				idx = 0;
		}
	}
	return nullptr;
}

template<typename K, typename V>
bool HashTableRemove(HashTable<K, V>* map, K* key)
{
	if (!map->Buckets || map->Count == 0)
		return false;

	uint32_t idx = HashTableHasher(key, sizeof(K), map->Capacity);
	while (true)
	{
		HashTableBucket<K, V>* bucket = &map->Buckets[idx];
		if (!bucket->IsUsed)
		{
			break;
		}
		else
		{
			if (*key == *bucket->Key)
			{
				while (true) // Move any entries after index closer to their ideal probe length.
				{
					uint32_t lastIdx = idx;
					++idx;
					if (idx == map->Capacity)
						idx = 0;

					HashTableBucket<K, V>* nextBucket = &map->Buckets[idx];
					if (!nextBucket->IsUsed || nextBucket->ProbeLength == 0) // No more entires to move
					{
						map->Buckets[lastIdx].ProbeLength = 0;
						map->Buckets[lastIdx].IsUsed = 0;
						--map->Count;
						return true;
					}
					else
					{
						map->Buckets[lastIdx] = *nextBucket;
						map->Buckets[lastIdx].ProbeLength -= 1;
					}
				}
			}
			else
			{
				++idx;
				if (idx == map->Capacity)
					idx = 0; // continue searching till 0 or found equals key
			}
		}
	}
	return false;
}

template<typename K, typename V>
void HashTableForEach(HashTable<K, V>* map, void(*Fn)(K*, V*, void*), void* stackMemory)
{
	SASSERT(Fn);
	for (uint32_t i = 0; i < map->Capacity; ++i)
	{
		if (map->Buckets[i].IsUsed)
		{
			Fn(&map->Buckets[i].Key, &map->Buckets[i].Value, stackMemory);
		}
	}
}
