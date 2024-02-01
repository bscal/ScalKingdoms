#pragma once

#include "Core.h"
#include "Memory.h"
#include "Lib/String.h"

constant_var u32 HASHMAPSTR_NOT_FOUND = UINT32_MAX;
constant_var size_t HASHMAPSTR_CAPACITY = 8;
constant_var size_t HASHMAPSTR_RESIZE = 2;
constant_var float HASHMAPSTR_LOADFACTOR = .85f;

struct HashStrSlot
{
	String String;
	u32 Hash;
	u16 ProbeLength;
	bool IsUsed;
};

// Robin hood hashmap seperating Slots (probelength, tombstone), Keys, Values
template<typename K>
struct HashMapStr
{
	HashStrSlot* Buckets;
	K* Values;
	u32 Capacity;
	u32 Count;
	u32 MaxCount;
	SAllocator Allocator;
};

template<typename K>
void HashMapStrInitialize(HashMapStr<K>* map, u32 capacity, SAllocator allocator)
{
	SAssert(map);
	SAssert(IsAllocatorValid(allocator));
	
	map->Allocator = allocator;
	if (capacity > 0)
	{
		capacity = (u32)ceilf((float)capacity / HASHMAPSTR_LOADFACTOR);
		HashMapStrReserve(map, capacity);
	}
}

template<typename K>
void HashMapStrReserve(HashMapStr<K>* map, uint32_t capacity)
{
	SAssert(map);
	SAssert(IsAllocatorValid(map->Allocator));

	if (capacity == 0)
		capacity = HASHMAPSTR_CAPACITY;

	if (capacity <= map->Capacity)
		return;

	if (!IsPowerOf2_32(capacity))
		capacity = AlignPowTwo32(capacity);

	if (map->Buckets)
	{
		HashMapStr<K> tmpMap = {};
		tmpMap.Allocator = map->Allocator;
		HashMapStrReserve(&tmpMap, capacity);

		for (u32 i = 0; i < map->Capacity; ++i)
		{
			if (map->Buckets[i].IsUsed)
			{
				HashMapStrSet(&tmpMap, map->Buckets[i].String, map->Values + i);
			}
		}

		HashMapStrFree(map);

		SAssert(map->Count == tmpMap.Count);
		*map = tmpMap;
	}
	else
	{
		map->Capacity = capacity;
		map->MaxCount = (u32)((float)map->Capacity * HASHMAPSTR_LOADFACTOR);
		map->Buckets = (HashStrSlot*)SCalloc(map->Allocator, sizeof(HashStrSlot) * map->Capacity);
		map->Values = (K*)SAlloc(map->Allocator, sizeof(K) * map->Capacity);
	}
}

template<typename K>
void HashMapStrClear(HashMapStr<K>* map)
{
	memset(map->Buckets, 0, sizeof(HashStrSlot) * map->Capacity);
	map->Count = 0;
}

template<typename K>
void HashMapStrFree(HashMapStr<K>* map)
{
	SFree(map->Allocator, map->Buckets);
	SFree(map->Allocator, map->Values);
	map->Capacity = 0;
	map->Count = 0;
}

template<typename K>
void HashMapStrSet(HashMapStr<K>* map, const String key, const K* value)
{
	HashMapStrSet_Internal(map, key, value);
}

template<typename K>
void HashMapStrReplace(HashMapStr<K>* map, const String key, const K* value)
{
	HashMapStrSetResults res = HashMapStrSet_Internal(map, key, value);
	if (res.Contained)
	{
		map->Values[res.Index] = *value;
	}

}

template<typename K>
K* HashMapStrSetZeroed(HashMapStr<K>* map, const String key)
{
	K tmpValue = {};
	HashMapStrSetResults res = HashMapStrSet_Internal(map, key, *tmpValue);
	K* ptr = map->Values + res.Index;
	return ptr;
}

template<typename K>
u32 HashMapStrFind(HashMapStr<K>* map, const String key)
{
	SAssert(map);
	SAssert(key);
	SAssert(IsAllocatorValid(map->Allocator));

	if (!map->Buckets || map->Count == 0)
		return HASHMAPSTR_NOT_FOUND;

	u32 hash = HashString(key);
	u32 idx = hash & (map->Capacity - 1);
	u32 probeLength = 0;
	while (true)
	{
		HashStrSlot slot = map->Buckets[idx];
		if (!slot.IsUsed || probeLength > slot.ProbeLength)
			return HASHMAPSTR_NOT_FOUND;
		else if (slot.Hash == hash && StringAreEqual(key, slot.String))
			return idx;
		else
		{
			++probeLength;
			++idx;
			if (idx == map->Capacity)
				idx = 0;
		}
	}
	SAssertMsg(false, "Shouldn't be run");
	return HASHMAPSTR_NOT_FOUND;
}

template<typename K>
K* HashMapStrGet(HashMapStr<K>* map, const String key)
{
	u32 idx = HashMapStrFind(map, key);
	if (idx == HASHMAPSTR_NOT_FOUND)
		return nullptr;
	else
		return map->Values + idx;
}

template<typename K>
bool HashMapStrRemove(HashMapStr<K>* map, const String key)
{
	SAssert(map);
	SAssert(key);
	SAssert(IsAllocatorValid(map->Allocator));

	if (!map->Buckets || map->Count == 0)
		return false;

	u32 hash = HashString(key);
	u32 idx = hash & (map->Capacity - 1);
	while (true)
	{
		HashStrSlot slot = map->Buckets[idx];
		if (!slot.IsUsed)
		{
			return false;
		}
		else if (slot.Hash == hash && StringAreEqual(key, slot.String))
		{
			while (true) // Move any entries after index closer to their ideal probe length.
			{
				uint32_t lastIdx = idx;
				++idx;
				if (idx == map->Capacity)
					idx = 0;

				HashStrSlot nextSlot = map->Buckets[idx];
				if (!nextSlot.IsUsed || nextSlot.ProbeLength == 0) // No more entires to move
				{
					map->Buckets[lastIdx].ProbeLength = 0;
					map->Buckets[lastIdx].IsUsed = 0;
					--map->Count;
					return true;
				}
				else
				{
					map->Buckets[lastIdx].String = nextSlot.String;
					map->Buckets[lastIdx].Hash = nextSlot.Hash;
					map->Buckets[lastIdx].ProbeLength = nextSlot.ProbeLength - 1;
					map->Values[lastIdx] = map->Values[idx];
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
	SAssertMsg(false, "shouldn't be called");
	return false;
}

struct HashMapStrSetResults
{
	u32 Index;
	bool Contained;
};

template<typename K>
HashMapStrSetResults
HashMapStrSet_Internal(HashMapStr<K>* map, const String key, const K* value)
{
	SAssert(map);
	SAssert(key);
	SAssert(IsAllocatorValid(map->Allocator));

	if (map->Count >= map->MaxCount)
	{
		HashMapStrReserve(map, map->Capacity * HASHMAPSTR_NOT_FOUND);
	}

	SAssert(map->Buckets);
	SAssert(map->Values);

	HashStrSlot swapSlot = {};
	swapSlot.String = key;
	swapSlot.IsUsed = true;

	swapSlot.Hash = HashString(key);
	u32 idx = swapSlot.Hash & (map->Capacity - 1);

	K swapValue = *value;

	HashMapStrSetResults res;
	res.Index = HASHMAPSTR_NOT_FOUND;
	res.Contained = false;

	u16 probeLength = 0;
	while (true)
	{
		HashStrSlot* slot = &map->Buckets[idx];
		if (!slot->IsUsed) // Bucket is not used
		{
			// Found an open spot. Insert and stops searching
			*slot = swapSlot;
			slot->ProbeLength = probeLength;
			map->Values[idx] = swapValue;
			++map->Count;

			if (res.Index == HASHMAPSTR_NOT_FOUND)
				res.Index = idx;

			return res;
		}
		else
		{
			if (slot->Hash == swapSlot.Hash && StringAreEqual(swapSlot.String, slot->String))
				return { idx, true };

			if (probeLength > slot->ProbeLength)
			{
				if (res.Index == HASHMAPSTR_NOT_FOUND)
					res.Index = idx;

				// Swap probe lengths and buckets
				Swap(slot->ProbeLength, probeLength, u16);
				Swap(swapSlot, *slot, HashStrSlot);
				Swap(swapValue, map->Values[idx], K);
			}

			++probeLength;
			++idx;
			if (idx == map->Capacity)
				idx = 0;

			SAssertMsg(probeLength <= UINT16_MAX, "probeLength is > UINT16_MAX, using bad hash?");
		}
	}
	return res;
}
