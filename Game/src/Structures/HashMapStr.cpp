#include "HashMapStr.h"

#include "Utils.h"

constexpr internal_var u32 DEFAULT_CAPACITY = 2;
constexpr internal_var u32 DEFAULT_RESIZE = 2;
constexpr internal_var float DEFAULT_LOADFACTOR = .85f;

#define ToIdx(hash, cap) ((u32)((hash) & (u64)((cap) - 1)))
#define IndexArray(arr, idx, stride) (((u8*)(arr)) + ((idx) * (stride)))
#define IndexValue(hashmap, idx) (IndexArray(hashmap->Values, idx, hashmap->ValueStride))

struct HashMapStrSetResults
{
	u32 Index;
	bool Contained;
};

internal HashMapStrSetResults
HashMapStrSet_Internal(HashMapStr* map, const String key, const void* value);

void 
HashMapStrInitialize(HashMapStr* map, u32 valueStride, u32 capacity, SAllocator alloc)
{
	SAssert(map);
	SAssert(IsAllocatorValid(alloc));
	SAssert(valueStride > 0);
	
	map->Alloc = alloc;
	map->ValueStride = valueStride;
	if (capacity > 0)
	{
		capacity = (u32)ceilf((float)capacity / DEFAULT_LOADFACTOR);
		HashMapStrReserve(map, capacity);
	}
}

void 
HashMapStrReserve(HashMapStr* map, uint32_t capacity)
{
	SAssert(map);
	SAssert(IsAllocatorValid(map->Alloc));
	SAssert(map->ValueStride > 0);

	if (capacity == 0)
		capacity = DEFAULT_CAPACITY;

	if (capacity <= map->Capacity)
		return;

	if (!IsPowerOf2_32(capacity))
		capacity = AlignPowTwo32(capacity);

	if (map->Buckets)
	{
		HashMapStr tmpMap = {};
		tmpMap.ValueStride = map->ValueStride;
		tmpMap.Alloc = map->Alloc;
		HashMapStrReserve(&tmpMap, capacity);

		for (u32 i = 0; i < map->Capacity; ++i)
		{
			if (map->Buckets[i].IsUsed)
			{
				HashMapStrSet(&tmpMap, map->Buckets[i].String, IndexValue(map, i));
			}
		}

		SFree(map->Alloc, map->Buckets);
		SFree(map->Alloc, map->Values);

		SAssert(map->Count == tmpMap.Count);
		*map = tmpMap;
	}
	else
	{
		map->Capacity = capacity;
		map->MaxCount = (u32)((float)map->Capacity * DEFAULT_LOADFACTOR);

		map->Buckets = (HashStrSlot*)SCalloc(map->Alloc, sizeof(HashStrSlot) * map->Capacity);
		//memset(map->Buckets, 0, sizeof(HashStrSlot) * map->Capacity);

		map->Values = SAlloc(map->Alloc, map->ValueStride * map->Capacity);
	}
}

void HashMapStrClear(HashMapStr* map)
{
	memset(map->Buckets, 0, sizeof(HashStrSlot) * map->Capacity);
	map->Count = 0;
}

void HashMapStrFree(HashMapStr* map)
{
	//SFree(map->Alloc, map->Buckets);
	//SFree(map->Alloc, map->Values);
	map->Capacity = 0;
	map->Count = 0;
}

void
HashMapStrSet(HashMapStr* map, const String key, const void* value)
{
	HashMapStrSet_Internal(map, key, value);
}

void
HashMapStrReplace(HashMapStr* map, const String key, const void* value)
{
	HashMapStrSetResults res = HashMapStrSet_Internal(map, key, value);
	if (res.Contained)
		memcpy(IndexValue(map, res.Index), value, map->ValueStride);
}

void*
HashMapStrSetZeroed(HashMapStr* map, const String key)
{
	HashMapStrSetResults res = HashMapStrSet_Internal(map, key, nullptr);
	return IndexValue(map, res.Index);
}

uint32_t HashMapStrFind(HashMapStr* map, const String key)
{
	SAssert(map);
	SAssert(key);
	SAssert(IsAllocatorValid(map->Alloc));

	if (!map->Buckets || map->Count == 0)
		return HASHMAPSTR_NOT_FOUND;

	size_t length = StringLength(key);
	u32 hash = FNVHash32(key, length);
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

void*
HashMapStrGet(HashMapStr* map, const String key)
{
	u32 idx = HashMapStrFind(map, key);
	if (idx == HASHMAPSTR_NOT_FOUND)
		return nullptr;
	else
		return IndexValue(map, idx);
}

bool HashMapStrRemove(HashMapStr* map, const String key)
{
	SAssert(map);
	SAssert(key);
	SAssert(IsAllocatorValid(map->Alloc));

	if (!map->Buckets || map->Count == 0)
		return false;

	size_t length = StringLength(key);
	u32 hash = FNVHash32(key, length);
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
					memcpy(IndexValue(map, lastIdx), IndexValue(map, idx), map->ValueStride);
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

internal HashMapStrSetResults
HashMapStrSet_Internal(HashMapStr* map, const String key, const void* value)
{
	SAssert(map);
	SAssert(key);
	SAssert(IsAllocatorValid(map->Alloc));
	SAssert(map->ValueStride > 0);

	if (map->Count >= map->MaxCount)
	{
		HashMapStrReserve(map, map->Capacity * DEFAULT_RESIZE);
	}

	SAssert(map->Buckets);

	HashStrSlot swapSlot = {};
	swapSlot.String = key;
	swapSlot.IsUsed = true;

	size_t length = StringLength(key);
	u32 hash = FNVHash32(key, length);
	swapSlot.Hash = hash;
	u32 idx = hash & (map->Capacity - 1);;
	SAssert(map->ValueStride < Kilobytes(16));
	void* swapValue = StackAlloc(map->ValueStride);
	SAssert(swapValue);

	if (value)
	{
		memcpy(swapValue, value, map->ValueStride);
	}
	else
	{
		memset(swapValue, 0, map->ValueStride);
	}

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
			memcpy(IndexValue(map, idx), swapValue, map->ValueStride);
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
				{
					// Swaps byte by byte from current value to
					// value parameter memory;
					uint8_t* currentValue = IndexValue(map, idx);
					for (uint32_t i = 0; i < map->ValueStride; ++i)
					{
						uint8_t tmp = currentValue[i];
						currentValue[i] = ((uint8_t*)swapValue)[i];
						((uint8_t*)swapValue)[i] = tmp;
					}
				}
			}

			++probeLength;
			++idx;
			if (idx == map->Capacity)
				idx = 0;

			SAssertMsg(probeLength <= 127, "probeLength is > 127, using bad hash?");
		}
	}
	return res;
}
