#include "HashMap.h"

#include "Utils.h"

#define HashMapKeyIndex(hash, cap) (hash & (cap - 1))
#define HashMapSwap(V0, V1, T) T tmp = V0; V0 = V1; V1 = tmp
#define HashMapKeySize(hashmap) (hashmap->Capacity * sizeof(HashBucket))
#define HashMapValueSize(hashmap) (hashmap->Capacity * hashmap->Stride)

void HashMapInitialize(HashMap* map, uint32_t stride, uint32_t capacity, Allocator alloc)
{
	SASSERT(stride > 0);
	SASSERT(IsAllocatorValid(alloc));

	map->Stride = stride;
	map->Alloc = alloc;
	if (capacity > 0)
		HashMapReserve(map, capacity);
}

void HashMapReserve(HashMap* map, uint32_t capacity)
{
	SASSERT(IsAllocatorValid(map->Alloc));
	SASSERT(map->Stride > 0);

	if (capacity == 0)
		capacity = HASHMAP_DEFAULT_CAPACITY;

	if (capacity <= map->Capacity)
		return;

	if (!IsPowerOf2_32(capacity))
		capacity = AlignPowTwo32(capacity);

	if (map->Keys)
	{
		HashMap tmpMap = {};
		tmpMap.Stride = map->Stride;
		tmpMap.Alloc = map->Alloc;
		HashMapReserve(&tmpMap, capacity);

		for (uint32_t i = 0; i < map->Capacity; ++i)
		{
			if (map->Keys[i].IsUsed)
			{
				HashMapSet(&tmpMap, map->Keys[i].Hash, HashMapValuesIndex(map, i));
			}
		}
		
		GameFree(map->Alloc, map->Keys);
		GameFree(map->Alloc, map->Values);

		SASSERT(map->Count == tmpMap.Count);
		*map = tmpMap;
	}
	else
	{
		map->Capacity = capacity;

		size_t newKeysSize = HashMapKeySize(map);
		size_t newValuesSize = HashMapValueSize(map);

		map->Keys = (HashBucket*)GameMalloc(map->Alloc, newKeysSize);
		map->Values = (uint8_t*)GameMalloc(map->Alloc, newValuesSize);

		memset(map->Keys, 0, sizeof(newKeysSize));

		map->MaxCount = (uint32_t)((float)map->Capacity * HASHMAP_LOAD_FACTOR);
	}
}

void HashMapClear(HashMap* map)
{
	SClear(map->Keys, HashMapKeySize(map));
	map->Count = 0;
}

void HashMapDestroy(HashMap* map)
{
	GameFree(map->Alloc, map->Keys);
	GameFree(map->Alloc, map->Values);
	map->Capacity = 0;
	map->Count = 0;
}

uint32_t HashMapSet(HashMap* map, uint32_t hash, const void* value)
{
	return HashMapSetEx(map, hash, value, false);
}

uint32_t HashMapSetEx(HashMap* map, uint32_t hash, const void* value, bool replace)
{
	if (map->Count >= map->MaxCount)
	{
		HashMapReserve(map, map->Capacity * HASHMAP_DEFAULT_RESIZE);
	}

	SASSERT(map->Keys);
	SASSERT(map->Stride > 0);

	#define MAX_HASHMAP_STRIDE 16 //kbs
	if (map->Stride >= Kilobytes(MAX_HASHMAP_STRIDE))
	{
		SError("Sizeof hashmap type > %dkbs. Failing to insert", MAX_HASHMAP_STRIDE);
		return HASHMAP_NOT_FOUND;
	}

	void* swapMemory = _alloca(map->Stride);
	if (!swapMemory)
	{
		SError("HashMapPut _alloca returned NULL!");
		return HASHMAP_NOT_FOUND;
	}

	if (value)
		memcpy(swapMemory, value, map->Stride);
	else
	 	memset(swapMemory, 0, map->Stride);

	uint32_t insertedIndex = HASHMAP_NOT_FOUND;
	uint32_t swapHash = hash;
	uint32_t probeLength = 0;

	uint32_t idx = HashMapKeyIndex(hash, map->Capacity);
	while (true)
	{
		HashBucket* bucket = &map->Keys[idx];
		if (!bucket->IsUsed) // Bucket is not used
		{
			bucket->Hash = swapHash;
			bucket->ProbeLength = probeLength;
			bucket->IsUsed = 1;
			memcpy(HashMapValuesIndex(map, idx), swapMemory, map->Stride);
			++map->Count;

			if (insertedIndex == HASHMAP_NOT_FOUND)
				insertedIndex = idx;

			break;
		}
		else
		{
			if (replace && bucket->Hash == hash)
			{
				SCopy(HashMapValuesIndex(map, idx), value, map->Stride);
				return idx;
			}

			if (probeLength > bucket->ProbeLength)
			{
				if (insertedIndex == HASHMAP_NOT_FOUND)
					insertedIndex = idx;

				{
					HashMapSwap(bucket->ProbeLength, probeLength, uint32_t);
				}
				{
					HashMapSwap(bucket->Hash, swapHash, uint32_t);
				}
				{
					// Swaps byte by byte from current value to
					// value parameter memory;
					uint8_t* currentValue = HashMapValuesIndex(map, idx);
					for (uint32_t i = 0; i < map->Stride; ++i)
					{
						uint8_t tmp = currentValue[i];
						currentValue[i] = ((uint8_t*)swapMemory)[i];
						((uint8_t*)swapMemory)[i] = tmp;
					}
				}
			}

			++probeLength;
			++idx;
			if (idx == map->Capacity)
				idx = 0;
		}
	}

	return insertedIndex;
}

void* HashMapSetNew(HashMap* map, uint32_t hash)
{
	uint32_t idx = HashMapSet(map, hash, nullptr);
	if (idx != HASHMAP_NOT_FOUND)
		return HashMapValuesIndex(map, idx);
	else
		return nullptr;
}

uint32_t HashMapIndex(HashMap* map, uint32_t hash)
{
	if (!map->Keys || map->Count == 0)
		return HASHMAP_NOT_FOUND;

	uint32_t probeLength = 0;
	uint32_t idx = HashMapKeyIndex(hash, map->Capacity);
	while (true)
	{
		HashBucket bucket = map->Keys[idx];
		if (!bucket.IsUsed || probeLength > bucket.ProbeLength)
			return HASHMAP_NOT_FOUND;
		else if (hash == bucket.Hash)
			return idx;
		else
		{
			++probeLength;
			++idx;
			if (idx == map->Capacity)
				idx = 0;
		}
	}
	return HASHMAP_NOT_FOUND;
}


bool HashMapRemove(HashMap* map, uint32_t hash)
{
	if (!map->Keys || map->Count == 0)
		return false;

	uint32_t idx = HashMapKeyIndex(hash, map->Capacity);
	while (true)
	{
		HashBucket* bucket = &map->Keys[idx];
		if (!bucket->IsUsed)
		{
			break;
		}
		else
		{
			if (hash == bucket->Hash)
			{
				while (true) // Move any entries after index closer to their ideal probe length.
				{
					uint32_t lastIdx = idx;
					++idx;
					if (idx == map->Capacity)
						idx = 0;

					HashBucket* nextBucket = &map->Keys[idx];
					if (!nextBucket->IsUsed || nextBucket->ProbeLength == 0) // No more entires to move
					{
						map->Keys[lastIdx].ProbeLength = 0;
						map->Keys[lastIdx].IsUsed = 0;
						--map->Count;
						return true;
					}
					else
					{
						map->Keys[lastIdx].Hash = nextBucket->Hash;
						map->Keys[lastIdx].ProbeLength = nextBucket->ProbeLength - 1;
						*HashMapValuesIndex(map, lastIdx) = *HashMapValuesIndex(map, idx);
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

void HashMapForEach(HashMap* map, void(*Fn)(uint32_t, void*, void*), void* stackMemory)
{
	SASSERT(Fn);
	for (uint32_t i = 0; i < map->Capacity; ++i)
	{
		if (map->Keys[i].IsUsed)
		{
			Fn(map->Keys[i].Hash, (void*)HashMapValuesIndex(map, i), stackMemory);
		}
	}
}
