#include "HashMap.h"

#include "Utils.h"

constexpr internal_var u32 DEFAULT_CAPACITY = 2;
constexpr internal_var u32 DEFAULT_RESIZE = 2;
constexpr internal_var float DEFAULT_LOADFACTOR = .85f;

#define Compare(hashmap, v0, v1) (hashmap->CompareFunc(v0, v1))

#define ToIdx(hash, cap) ((u32)((hash) & (u64)((cap) - 1)))
#define IndexArray(arr, idx, stride) (((u8*)(arr)) + ((idx) * (stride)))
#define IndexKey(hashmap, idx) (IndexArray(hashmap->Keys, idx, hashmap->KeyStride))
#define IndexValue(hashmap, idx) (IndexArray(hashmap->Values, idx, hashmap->ValueStride))

struct HashMapSetResults
{
    u32 Index;
    bool Contained;
};

internal HashMapSetResults
HashMapSet_Internal(HashMap* map, const void* key, const void* value);

void HashMapInitialize(HashMap* map, HashMapCompare compareFunc, 
	u32 keyStride, u32 valueStride, u32 capacity, SAllocator alloc)
{
	SAssert(map);
	SAssert(compareFunc);
	SAssert(IsAllocatorValid(alloc));
    SAssert(keyStride > 0);
    SAssert(valueStride > 0);

	map->CompareFunc = compareFunc;
	map->Alloc = alloc;
	map->KeyStride = keyStride;
    map->ValueStride = valueStride;
	if (capacity > 0)
	{
		capacity = (u32)ceilf((float)capacity / DEFAULT_LOADFACTOR);
		HashMapReserve(map, capacity);
	}
}

void HashMapReserve(HashMap* map, uint32_t capacity)
{
	SAssert(map);
	SAssert(IsAllocatorValid(map->Alloc));
    SAssert(map->KeyStride > 0);
    SAssert(map->ValueStride > 0);

	if (capacity == 0)
		capacity = DEFAULT_CAPACITY;

	if (capacity <= map->Capacity)
		return;

	if (!IsPowerOf2_32(capacity))
		capacity = AlignPowTwo32(capacity);

	if (map->Slots)
	{
		HashMap tmpMap = {};
		tmpMap.CompareFunc = map->CompareFunc;
		tmpMap.KeyStride = map->KeyStride;
        tmpMap.ValueStride = map->ValueStride;
		tmpMap.Alloc = map->Alloc;
		HashMapReserve(&tmpMap, capacity);

		for (u32 i = 0; i < map->Capacity; ++i)
		{
			if (map->Slots[i].IsUsed)
			{
				HashMapSet(&tmpMap, IndexKey(map, i), IndexValue(map, i));
			}
		}
		
		SFree(map->Alloc, map->Slots);
		SFree(map->Alloc, map->Keys);
        SFree(map->Alloc, map->Values);

		SAssert(map->Count == tmpMap.Count);
		*map = tmpMap;
	}
	else
	{
		map->Capacity = capacity;
		map->MaxCount = (u32)((float)map->Capacity * DEFAULT_LOADFACTOR);

		map->Slots = (HashMapSlot*)SAlloc(map->Alloc, sizeof(HashMapSlot) * map->Capacity);
		memset(map->Slots, 0, sizeof(HashMapSlot) * map->Capacity);

		map->Keys = SAlloc(map->Alloc, map->KeyStride * map->Capacity);

        map->Values = SAlloc(map->Alloc, map->ValueStride * map->Capacity);
	}
}

void HashMapClear(HashMap* map)
{
	memset(map->Slots, 0, sizeof(HashMapSlot) * map->Capacity);
	map->Count = 0;
}

void HashMapFree(HashMap* map)
{
	SFree(map->Alloc, map->Slots);
	SFree(map->Alloc, map->Keys);
    SFree(map->Alloc, map->Values);
	map->Capacity = 0;
	map->Count = 0;
}

void 
HashMapSet(HashMap* map, const void* key, const void* value)
{
	HashMapSet_Internal(map, key, value);
}

void 
HashMapReplace(HashMap* map, const void* key, const void* value)
{
	HashMapSetResults res = HashMapSet_Internal(map, key, value);
    if (res.Contained)
        memcpy(IndexValue(map, res.Index), value, map->ValueStride);
}

void* 
HashMapSetZeroed(HashMap* map, const void* key)
{
	HashMapSetResults res = HashMapSet_Internal(map, key, nullptr);
    return IndexValue(map, res.Index);
}

uint32_t HashMapFind(HashMap* map, const void* key)
{
	SAssert(map);
	SAssert(key);
	SAssert(map->CompareFunc);
	SAssert(IsAllocatorValid(map->Alloc));

	if (!map->Slots || map->Count == 0)
		return HASHMAPKV_NOT_FOUND;

	u32 idx = HashKey(key, map->KeyStride, map->Capacity);
	u32 probeLength = 0;
	while (true)
	{
		HashMapSlot slot = map->Slots[idx];
		if (!slot.IsUsed || probeLength > slot.ProbeLength)
			return HASHMAPKV_NOT_FOUND;
		else if (Compare(map, IndexKey(map, idx), key))
			return idx;
		else
		{
			++probeLength;
			++idx;
			if (idx == map->Capacity)
				idx = 0;
		}
	}
	return HASHMAPKV_NOT_FOUND;
}

void* 
HashMapGet(HashMap* map, const void* key)
{
	u32 idx = HashMapFind(map, key);
    if (idx == HASHMAPKV_NOT_FOUND)
        return nullptr;
    else
        return IndexValue(map, idx);
}

bool HashMapRemove(HashMap* map, const void* key)
{
	SAssert(map);
	SAssert(key);
	SAssert(map->CompareFunc);
	SAssert(IsAllocatorValid(map->Alloc));

	if (!map->Slots || map->Count == 0)
		return false;

	u32 idx = HashKey(key, map->KeyStride, map->Capacity);
	while (true)
	{
		HashMapSlot bucket = map->Slots[idx];
		if (!bucket.IsUsed)
		{
			return false;
		}
		else if (Compare(map, key, IndexKey(map, idx)))
		{
			while (true) // Move any entries after index closer to their ideal probe length.
			{
				uint32_t lastIdx = idx;
				++idx;
				if (idx == map->Capacity)
					idx = 0;

				HashMapSlot nextBucket = map->Slots[idx];
				if (!nextBucket.IsUsed || nextBucket.ProbeLength == 0) // No more entires to move
				{
					map->Slots[lastIdx].ProbeLength = 0;
					map->Slots[lastIdx].IsUsed = 0;
					--map->Count;
					return true;
				}
				else
				{
					map->Slots[lastIdx].ProbeLength = nextBucket.ProbeLength - 1;
                    memcpy(IndexKey(map, lastIdx), IndexKey(map, idx), map->KeyStride);
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

void HashMapForEach(HashMap* map, void(*Fn)(void*, void*, void*), void* stackMemory)
{
	SAssert(Fn);
	for (uint32_t i = 0; i < map->Capacity; ++i)
	{
		if (map->Slots[i].IsUsed)
		{
			Fn(IndexKey(map, i), IndexValue(map, i), stackMemory);
		}
	}
}

internal HashMapSetResults
HashMapSet_Internal(HashMap* map, const void* key, const void* value)
{
	SAssert(map);
	SAssert(key);
	SAssert(map->CompareFunc);
	SAssert(IsAllocatorValid(map->Alloc));
	SAssert(map->KeyStride > 0);
	SAssert(map->ValueStride > 0);
	SAssert(map->KeyStride + map->ValueStride < Kilobytes(16));

	if (map->Count >= map->MaxCount)
	{
		HashMapReserve(map, map->Capacity * DEFAULT_RESIZE);
	}

	SAssert(map->Slots);

    void* swapKey = StackAlloc(map->KeyStride);
    SAssert(swapKey);

    memcpy(swapKey, key, map->KeyStride);

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
	
	HashMapSetResults res;
    res.Index = HASHMAPKV_NOT_FOUND;
	res.Contained = false;

	u32 idx = HashKey(key, map->KeyStride, map->Capacity);
	u8 probeLength = 0;
	while (true)
	{
		HashMapSlot* bucket = &map->Slots[idx];
		if (!bucket->IsUsed) // Bucket is not used
		{
            bucket->IsUsed = true;
            bucket->ProbeLength = probeLength;
            memcpy(IndexKey(map, idx), swapKey, map->KeyStride);
			memcpy(IndexValue(map, idx), swapValue, map->ValueStride);

			++map->Count;

			if (res.Index == HASHMAPKV_NOT_FOUND)
				res.Index = idx;

			return res;
		}
		else
		{
			if (Compare(map, IndexKey(map, idx), swapKey))
				return { idx, true };

			if (probeLength > bucket->ProbeLength)
			{
                if (res.Index == HASHMAPKV_NOT_FOUND)
                    res.Index = idx;

				Swap(bucket->ProbeLength, probeLength, u8);
                {
					// Swaps byte by byte from current value to
					// value parameter memory;
					uint8_t* currentValue = IndexKey(map, idx);
					for (uint32_t i = 0; i < map->KeyStride; ++i)
					{
						uint8_t tmp = currentValue[i];
						currentValue[i] = ((uint8_t*)swapKey)[i];
						((uint8_t*)swapKey)[i] = tmp;
					}
				}
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

void* HashMapIndex(HashMap* map, u32 idx)
{
	return IndexValue(map, idx);
}
