#pragma once

#include "Core.h"
#include "Memory.h"
#include "Utils.h"

template<typename K, typename V>
struct HashMapTBucket
{
	K Key;
	u16 ProbeLength;
	bool IsUsed;
	V Value;
};

template<typename K, typename V>
struct HashMapT
{
	constexpr static uint32_t NOT_FOUND = UINT32_MAX;
	constexpr static uint32_t DEFAULT_CAPACITY = 2;
	constexpr static uint32_t DEFAULT_RESIZE = 2;
	constexpr static float DEFAULT_LOADFACTOR = 0.85f;

	HashMapTBucket<K, V>* Buckets;
	u32 Capacity;
	u32 Count;
	u32 MaxCount;
	Allocator Alloc;
};

template<typename K, typename V>
void HashMapTInitialize(HashMapT<K, V>* map, uint32_t capacity, float loadFactor, Allocator alloc)
{
	SASSERT(map);
	SASSERT(IsAllocatorValid(alloc));

	map->Alloc = alloc;
	if (capacity > 0)
		HashMapTReserve(map, capacity);
}

template<typename K, typename V>
void HashMapTReserve(HashMapT<K, V>* map, uint32_t capacity)
{
	SASSERT(map);
	SASSERT(IsAllocatorValid(map->Alloc));

	if (capacity == 0)
		capacity = DEFAULT_CAPACITY;

	if (capacity <= map->Capacity)
		return;

	if (!IsPowerOf2_32(capacity))
		capacity = AlignPowTwo32(capacity);

	if (map->Buckets)
	{
		HashMapT<K, V> tmpMap = {};
		tmpMap.Alloc = map->Alloc;
		HashMapTReserve<K, V>(&tmpMap, capacity);

		for (uint32_t i = 0; i < map->Capacity; ++i)
		{
			if (map->Buckets[i].IsUsed)
			{
				HashMapTSet<K, V>(&tmpMap, &map->Buckets[i].Key, &map->Buckets[i].Value);
			}
		}

		GameFree(map->Alloc, map->Buckets);

		SASSERT(map->Count == tmpMap.Count);
		*map = tmpMap;
	}
	else
	{
		map->Capacity = capacity;
		map->MaxCount = (uint32_t)((float)map->Capacity * DEFAULT_LOADFACTOR);

		size_t size = sizeof(HashMapTBucket<K, V>) * map->Capacity;

		map->Buckets = (HashMapTBucket<K, V>*)GameMalloc(map->Alloc, size);

		memset(map->Buckets, 0, size);
	}
}

template<typename K, typename V>
void HashMapTClear(HashMapT<K, V>* map)
{
	SASSERT(map);
	for (uint32_t i = 0; i < map->Capacity; ++i)
	{
		map->Buckets[i].ProbeLength = 0;
		map->Buckets[i].IsUsed = 0;
	}
	map->Count = 0;
}

template<typename K, typename V>
void HashMapTDestroy(HashMapT<K, V>* map)
{
	SASSERT(map);
	GameFree(map->Alloc, map->Buckets);
	map->Capacity = 0;
	map->Count = 0;
}

template<typename K, typename V>
uint32_t HashMapTSet(HashMapT<K, V>* map, K* key, V* value)
{
	SASSERT(map);
	SASSERT(key);
	SASSERT(IsAllocatorValid(map->Alloc));

	if (map->Count >= map->MaxCount)
	{
		HashMapTReserve(map, map->Capacity * DEFAULT_RESIZE);
	}

	SASSERT(map->Buckets);

	HashMapTBucket<K, V> swapBucket;
	swapBucket.Key = *key;
	swapBucket.ProbeLength = 0;
	swapBucket.IsUsed = 1;
	if (value)
		swapBucket.Value = *value;
	else
		swapBucket.Value = {};

	uint32_t insertedIndex = NOT_FOUND;
	uint32_t probeLength = 0;
	uint32_t idx = HashKey(key, sizeof(K), map->Capacity);
	while (true)
	{
		HashMapTBucket<K, V>* bucket = &map->Buckets[idx];
		if (!bucket->IsUsed) // Bucket is not used
		{
			if (insertedIndex == NOT_FOUND)
				insertedIndex = idx;

			// Found an open spot. Insert and stops searching
			*bucket = swapBucket;
			bucket->ProbeLength = probeLength;
			++map->Count;
			break;
		}
		else
		{
			if (bucket->Key == swapBucket.Key)
				return idx;

			if (probeLength > bucket->ProbeLength)
			{
				if (insertedIndex == NOT_FOUND)
					insertedIndex = idx;

				// Swap probe lengths and buckets
				swapBucket.ProbeLength = probeLength;
				probeLength = bucket->ProbeLength;
				HashMapTBucket<K, V> tmp = *bucket;
				*bucket = swapBucket;
				swapBucket = tmp;
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
uint32_t HashMapTSetNullValue(HashMapT<K, V>* map, K* key)
{
	return HashMapTSet<K, V>(map, key, nullptr);
}

template<typename K, typename V>
uint32_t HashMapTReplace(HashMapT<K, V>* map, K* key, V* value)
{
	uint32_t insertedIdx = HashMapTSet<K, V>(map, key, nullptr);
	SASSERT(insertedIdx != NOT_FOUND);
	map->Buckets[insertedIdx].Value = *value;
	return insertedIdx;
}

template<typename K, typename V>
uint32_t HashMapTFind(HashMapT<K, V>* map, K* key)
{
	SASSERT(map);
	SASSERT(key);
	SASSERT(IsAllocatorValid(map->Alloc));
	SASSERT(*key == *key);

	if (!map->Buckets || map->Count == 0)
		return NOT_FOUND;

	SASSERT(map->Buckets);

	uint32_t probeLength = 0;
	uint32_t idx = HashKey(key, sizeof(K), map->Capacity);
	while (true)
	{
		HashMapTBucket<K, V>* bucket = &map->Buckets[idx];
		if (!bucket->IsUsed || probeLength > bucket->ProbeLength)
			return HASHMAPT_NOT_FOUND;
		else if (*key == bucket->Key)
			return idx;
		else
		{
			++probeLength;
			++idx;
			if (idx == map->Capacity)
				idx = 0;
		}
	}
	return NOT_FOUND;
}

template<typename K, typename V>
V* HashMapTGet(HashMapT<K, V>* map, K* key)
{
	uint32_t idx = HashMapTFind<K, V>(map, key);
	return (idx != NOT_FOUND) ? &map->Buckets[idx].Value : nullptr;
}

template<typename K, typename V>
bool HashMapTRemove(HashMapT<K, V>* map, K* key)
{
	SASSERT(map);
	SASSERT(key);

	if (!map->Buckets || map->Count == 0)
		return false;

	SASSERT(map->Buckets);

	uint32_t index = HashKey(key, sizeof(K), map->Capacity);
	while (true)
	{
		HashMapTBucket<K, V>* bucket = &map->Buckets[index];
		if (!bucket->IsUsed)
		{
			break; // No key found
		}
		else
		{
			if (*key == bucket->Key)
			{
				while (true) // Move any entries after index closer to their ideal probe length.
				{
					uint32_t lastIndex = index;
					if (++index == map->Capacity)
						index = 0;

					HashMapTBucket<K, V>* nextBucket = &map->Buckets[index];
					if (!nextBucket->IsUsed || nextBucket->ProbeLength == 0) // No more entires to move
					{
						map->Buckets[lastIndex].ProbeLength = 0;
						map->Buckets[lastIndex].IsUsed = false;
						--map->Count;
						return true;
					}
					else
					{
						--nextBucket->ProbeLength;
						map->Buckets[lastIndex] = *nextBucket;
					}
				}
			}
			else
			{
				++index;
				if (index == map->Capacity)
					index = 0; // continue searching till 0 or found equals key
			}
		}
	}
	return false;
}

template<typename K, typename V>
void HashMapTForEach(HashMapT<K, V>* map, void(*Fn)(K*, V*, void*), void* stackMemory)
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
