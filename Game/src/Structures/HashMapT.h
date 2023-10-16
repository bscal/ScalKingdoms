#pragma once

#include "Core.h"
#include "Memory.h"
#include "Utils.h"

template<typename K, typename V>
struct HashMapTBucket
{
	V Value;
	K Key;
	u16 ProbeLength;
	bool IsUsed;
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
	SAllocator Alloc;
};

template<typename K, typename V>
void HashMapTInitialize(HashMapT<K, V>* map, uint32_t capacity, SAllocator alloc)
{
	SAssert(map);
	SAssert(IsAllocatorValid(alloc));

	map->Alloc = alloc;
	if (capacity > 0)
	{
		capacity = (u32)ceilf((float)capacity / HashMapT<K, V>::DEFAULT_LOADFACTOR);
		HashMapTReserve(map, capacity);
	}
}

template<typename K, typename V>
void HashMapTReserve(HashMapT<K, V>* map, uint32_t capacity)
{
	SAssert(map);
	SAssert(IsAllocatorValid(map->Alloc));

	if (capacity == 0)
		capacity = HashMapT<K, V>::DEFAULT_CAPACITY;

	if (capacity <= map->Capacity)
		return;

	if (!IsPowerOf2_32(capacity))
		capacity = AlignPowTwo32Ceil(capacity);

	if (map->Buckets)
	{
		PushMemoryPointer(map->Buckets);

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

		SFree(map->Alloc, map->Buckets);

		SAssert(map->Count == tmpMap.Count);
		SAssert(map->Capacity < tmpMap.Capacity);
		*map = tmpMap;

		PopMemoryPointer();
	}
	else
	{
		map->Capacity = capacity;
		map->MaxCount = (uint32_t)((float)map->Capacity * HashMapT<K, V>::DEFAULT_LOADFACTOR);

		SAssert(IsPowerOf2_32(map->Capacity));
		SAssert(map->MaxCount < map->Capacity);
		
		size_t size = sizeof(HashMapTBucket<K, V>) * map->Capacity;
		SAssert(size > 0);
		map->Buckets = (HashMapTBucket<K, V>*)SAlloc(map->Alloc, size);
		SAssert(map->Buckets);
		memset(map->Buckets, 0, size);
	}
}

template<typename K, typename V>
void HashMapTClear(HashMapT<K, V>* map)
{
	SAssert(map);
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
	SAssert(map);
	SAssert(map->Buckets);
	SAssert(IsAllocatorValid(map->Alloc));
	SFree(map->Alloc, map->Buckets);
	map->Capacity = 0;
}

template<typename K, typename V>
uint32_t HashMapTSet(HashMapT<K, V>* map, K* key, V* value)
{
	SAssert(map);
	SAssert(key);
	SAssert(IsAllocatorValid(map->Alloc));
	SAssert(*key == *key);

	if (map->Count >= map->MaxCount)
	{
		HashMapTReserve(map, map->Capacity * HashMapT<K, V>::DEFAULT_RESIZE);
	}

	SAssert(map->Buckets);

	HashMapTBucket<K, V> swapBucket;
	swapBucket.Key = *key;
	swapBucket.ProbeLength = 0;
	swapBucket.IsUsed = true;
	if (value)
		swapBucket.Value = *value;
	else
		swapBucket.Value = {};

	uint32_t insertedIndex = HashMapT<K, V>::NOT_FOUND;
	uint16_t probeLength = 0;
	uint32_t idx = (uint32_t)HashAndMod(key, map->Capacity);
	while (true)
	{
		HashMapTBucket<K, V>* bucket = &map->Buckets[idx];
		if (!bucket->IsUsed) // Bucket is not used
		{
			if (insertedIndex == HashMapT<K, V>::NOT_FOUND)
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
				if (insertedIndex == HashMapT<K, V>::NOT_FOUND)
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
	SAssert(insertedIdx != UINT32_MAX);
	map->Buckets[insertedIdx].Value = *value;
	return insertedIdx;
}

template<typename K, typename V>
uint32_t HashMapTFind(HashMapT<K, V>* map, K* key)
{
	SAssert(map);
	SAssert(key);
	SAssert(IsAllocatorValid(map->Alloc));
	SAssert(*key == *key);
	SAssert(map->Buckets);

	if (!map->Buckets)
		return HashMapT<K, V>::NOT_FOUND;

	uint32_t probeLength = 0;
	uint32_t idx = (uint32_t)HashAndMod(key, map->Capacity);
	while (true)
	{
		HashMapTBucket<K, V>* bucket = &map->Buckets[idx];
		if (!bucket->IsUsed || probeLength > bucket->ProbeLength)
			return HashMapT<K, V>::NOT_FOUND;
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
	SAssertMsg(false, "Shouldn't be executed");
	return HashMapT<K,V>::NOT_FOUND;
}

template<typename K, typename V>
V* HashMapTGet(HashMapT<K, V>* map, K* key)
{
	uint32_t idx = HashMapTFind<K, V>(map, key);
	return (idx != HashMapT<K,V>::NOT_FOUND) ? &map->Buckets[idx].Value : nullptr;
}

template<typename K, typename V>
bool HashMapTRemove(HashMapT<K, V>* map, const K* key)
{
	SAssert(map);
	SAssert(key);
	SAssert(*key == *key);

	if (!map->Buckets || map->Count == 0)
		return false;

	SAssert(map->Buckets);

	uint32_t index = (uint32_t)HashAndMod(key, map->Capacity);
	while (true)
	{
		HashMapTBucket<K, V>* bucket = &map->Buckets[index];
		if (!bucket->IsUsed)
		{
			return false; // No key found
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
	SAssertMsg(false, "Shouldn't be executed");
	return false;
}

template<typename K, typename V>
void HashMapTForEach(HashMapT<K, V>* map, void(*Fn)(K*, V*, void*), void* stackMemory)
{
	SAssert(Fn);
	u32 processedCount = 0;
	for (u32 i = 0; i < map->Capacity; ++i)
	{
		if (map->Buckets[i].IsUsed)
		{
			Fn(&map->Buckets[i].Key, &map->Buckets[i].Value, stackMemory);
			if (++processedCount == map->Count)
				break;
		}
	}
}

template<typename K, typename V>
void HashMapTPrint(HashMapT<K, V>* map, const char*(*FmtKey)(K*), const char*(*FmtValue)(V*))
{
	SAssert(map);
	SAssert(FmtKey);
	SAssert(FmtValue);
	for (u32 i = 0; i < map->Capacity; ++i)
	{
		if (map->Buckets[i].IsUsed)
		{
			SDebugLog("\t[%d] %s (probe: %d) = %s",
				i,
				FmtKey(&map->Buckets[i].Key),
				map->Buckets[i].ProbeLength,
				FmtValue(&map->Buckets[i].Value));
		}
		else
		{
			SDebugLog("\t[%d] NULL", i);
		}
	}
}
