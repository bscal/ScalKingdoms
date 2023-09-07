#pragma once

#include "Core.h"
#include "Memory.h"
#include "Utils.h"

template<typename K>
struct HashSetTBucket
{
	K Key;
	uint32_t ProbeLength : 31;
	uint32_t IsUsed : 1;
};

template<typename K>
struct HashSetT
{
	constexpr static uint32_t DEFAULT_CAPACITY = 2;
	constexpr static uint32_t DEFAULT_RESIZE = 2;
	constexpr static float DEFAULT_LOADFACTOR = 0.85f;

	HashSetTBucket<K>* Keys;
	uint32_t Capacity;
	uint32_t Count;
	uint32_t MaxCount;
	Allocator Alloc;
};

template<typename K>
void
HashSetTInitialize(HashSetT<K>* set, uint32_t capacity, Allocator allocator)
{
	SASSERT(set);
	SASSERT(IsAllocatorValid(allocator));

	set->Alloc = allocator;
	if (capacity > 0)
		HashSetTReserve(set, capacity);
}

template<typename K>
void 
HashSetTReserve(HashSetT<K>* set, uint32_t capacity)
{
	SASSERT(set);
	SASSERT(IsAllocatorValid(set->Alloc));

	if (capacity == 0)
		capacity = HashSetT<K>::DEFAULT_CAPACITY;

	if (capacity <= set->Capacity)
		return;

	if (!IsPowerOf2_32(capacity))
		capacity = AlignPowTwo32(capacity);

	if (set->Keys)
	{
		HashSetT<K> tmpSet = {};
		tmpSet.Alloc = set->Alloc;
		HashSetTReserve(&tmpSet, capacity);

		for (uint32_t i = 0; i < set->Capacity; ++i)
		{
			if (set->Keys[i].IsUsed)
			{
				HashSetTSet(&tmpSet, &set->Keys[i].Key);
			}
		}

		GameFree(set->Alloc, set->Keys);

		SASSERT(set->Keys);
		SASSERT(set->Count == tmpSet.Count);
		SASSERT(set->Capacity < tmpSet.Capacity);
		*set = tmpSet;
	}
	else
	{
		set->Capacity = capacity;
		set->MaxCount = (uint32_t)((float)set->Capacity * HashSetT<K>::DEFAULT_LOADFACTOR);

		SASSERT(IsPowerOf2_32(set->Capacity));
		SASSERT(set->MaxCount < set->Capacity);

		size_t size = sizeof(HashSetTBucket<K>) * set->Capacity;
		SASSERT(size > 0);
		set->Keys = (HashSetTBucket<K>*)GameMalloc(set->Alloc, size);
		SASSERT(set->Keys);
		memset(set->Keys, 0, size);
	}
}

template<typename K>
void 
HashSetTClear(HashSetT<K>* set)
{
	SASSERT(set);
	size_t size = sizeof(HashSetTBucket<K>) * set->Capacity;
	memset(set->Keys, 0, size);
	set->Count = 0;
}

template<typename K>
void 
HashSetTDestroy(HashSetT<K>* set)
{
	SASSERT(set);
	GameFree(set->Alloc, set->Keys);
	*set = {};
}

template<typename K>
bool 
HashSetTSet(HashSetT<K>* set, const K* key)
{
	SASSERT(set);
	SASSERT(key);
	SASSERT(IsAllocatorValid(set->Alloc));
	SASSERT(*key == *key);

	if (set->Count >= set->MaxCount)
	{
		HashSetTReserve(set, set->Capacity * HashSetT<K>::DEFAULT_RESIZE);
	}

	SASSERT(set->Keys);

	K swapKey = *key;
	uint32_t idx = HashKey(key, sizeof(K), set->Capacity);
	uint32_t probeLength = 0;
	while (true)
	{
		HashSetTBucket<K>* bucket = &set->Keys[idx];
		if (!bucket->IsUsed) // Bucket is not used
		{
			bucket->Key = swapKey;
			bucket->ProbeLength = probeLength;
			bucket->IsUsed = 1;
			++set->Count;
			return true;
		}
		else
		{
			if (bucket->Key == swapKey)
				return false;

			if (probeLength > bucket->ProbeLength)
			{
				Swap(bucket->ProbeLength, probeLength, uint32_t);
				Swap(bucket->Key, swapKey, K);
			}

			++probeLength;
			++idx;
			if (idx == set->Capacity)
				idx = 0;
		}
	}
	return false;
}

template<typename K>
bool 
HashSetTContains(HashSetT<K>* set, K* key)
{
	SASSERT(set);
	SASSERT(key);
	SASSERT(*key == *key);
	if (!key || !set->Keys || set->Count == 0)
		return false;

	uint32_t probeLength = 0;
	uint32_t idx = HashKey(key, sizeof(K), set->Capacity);
	while (true)
	{
		HashSetTBucket<K>* bucket = &set->Keys[idx];
		if (!bucket->IsUsed || probeLength > bucket->ProbeLength)
			return false;
		else if (*key == bucket->Key)
			return true;
		else
		{
			++probeLength;
			++idx;
			if (idx == set->Capacity)
				idx = 0;
		}
	}
	SASSERT_MSG(false, "Shouldn't be executed");
	return false;
}

template<typename K>
bool 
HashSetTRemove(HashSetT<K>* set, K* key)
{
	SASSERT(set);
	SASSERT(key);
	SASSERT(*key == *key);
	if (!set->Keys || set->Count == 0)
		return false;

	uint32_t idx = HashKey(key, sizeof(K), map->Capacity);
	while (true)
	{
		HashSetTBucket<K>* bucket = &set->Keys[idx];
		if (!bucket->IsUsed)
		{
			return false;
		}
		else
		{
			if (*key == bucket->Key)
			{
				while (true) // Move any entries after index closer to their ideal probe length.
				{
					uint32_t lastIdx = idx;
					++idx;
					if (idx == set->Capacity)
						idx = 0;

					HashSetTBucket<K>* nextBucket = &set->Keys[idx];
					if (!nextBucket->IsUsed || nextBucket->ProbeLength == 0) // No more entires to move
					{
						set->Keys[lastIdx].ProbeLength = 0;
						set->Keys[lastIdx].IsUsed = 0;
						--set->Count;
						return true;
					}
					else
					{
						set->Keys[lastIdx].Key = nextBucket->Key;
						set->Keys[lastIdx].ProbeLength = nextBucket->ProbeLength - 1;
					}
				}
			}
			else
			{-
				++idx;
				if (idx == set->Capacity)
					idx = 0; // continue searching till 0 or found equals key
			}
		}
	}
	SASSERT_MSG(false, "Shouldn't be executed");
	return false;
}
