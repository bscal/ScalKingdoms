#include "HashMapKV.h"

#include "Utils.h"

constexpr global_var u32 DEFAULT_CAPACITY = 2;
constexpr global_var u32 DEFAULT_RESIZE = 2;
constexpr global_var float DEFAULT_LOADFACTOR = .85f;

#define HashMapCompare(hashmap, v0, v1) (zpl_memcompare(v0, v1, hashmap->KeyStride))

#define ToIdx(hash, cap) ((u32)((hash) & (u64)((cap) - 1)))
#define IndexArray(arr, idx, stride) ((u8*)(arr) + ((idx) * (stride)))
#define IndexKey(hashmap, idx) (IndexArray(hashmap->Keys, idx, hashmap->KeyStride))
#define IndexValue(hashmap, idx) (IndexArray(hashmap->Values, idx, hashmap->ValueStride))

struct HashKVSlot
{
	u8 ProbeLength : 7;
	u8 IsUsed : 1;
};

struct HashMapKVSetResults
{
    u32 Index;
    bool Contained;
};

internal HashMapKVSetResults
HashMapKVSet_Internal(HashMapKV* map, const void* key, const void* value);

void HashMapKVInitialize(HashMapKV* map, Allocator alloc, u32 keyStride, u32 valueStride, u32 capacity)
{
	SASSERT(map);
	SASSERT(IsAllocatorValid(alloc));
    SASSERT(keyStride > 0);
    SASSERT(valueStride > 0);

	map->Alloc = alloc;
	map->KeyStride = keyStride;
    map->ValueStride = valueStride;
	if (capacity > 0)
		HashMapKVReserve(map, capacity);
}

void HashMapKVReserve(HashMapKV* map, uint32_t capacity)
{
	SASSERT(map);
	SASSERT(IsAllocatorValid(map->Alloc));
    SASSERT(map->KeyStride > 0);
    SASSERT(map->ValueStride > 0);

	if (capacity == 0)
		capacity = DEFAULT_CAPACITY;

	if (capacity <= map->Capacity)
		return;

	if (!IsPowerOf2_32(capacity))
		capacity = AlignPowTwo32(capacity);

	if (map->Slots)
	{
		HashMapKV tmpMap = {};
		tmpMap.KeyStride = map->KeyStride;
        tmpMap.ValueStride = map->ValueStride;
		tmpMap.Alloc = map->Alloc;
		HashMapKVReserve(&tmpMap, capacity);

		for (u32 i = 0; i < map->Capacity; ++i)
		{
			if (map->Slots[i].IsUsed)
			{
				HashMapKVSet(&tmpMap, IndexKey(map, i), IndexValue(map, i));
			}
		}
		
		GameFree(map->Alloc, map->Slots);
		GameFree(map->Alloc, map->Keys);
        GameFree(map->Alloc, map->Values);

		SASSERT(map->Count == tmpMap.Count);
		*map = tmpMap;
	}
	else
	{
		map->Capacity = capacity;
		map->MaxCount = (u32)((float)map->Capacity * DEFAULT_LOADFACTOR);

		map->Slots = (HashKVSlot*)GameMalloc(map->Alloc, sizeof(HashKVSlot) * map->Capacity);
		memset(map->Slots, 0, sizeof(HashKVSlot) * map->Capacity);
		map->Values = GameMalloc(map->Alloc, map->KeyStride * map->Capacity);
        map->Values = GameMalloc(map->Alloc, map->ValueStride * map->Capacity);
	}
}

void HashMapKVClear(HashMapKV* map)
{
	memset(map->Slots, 0, sizeof(HashKVSlot) * map->Capacity);
	map->Count = 0;
}

void HashMapKVFree(HashMapKV* map)
{
	GameFree(map->Alloc, map->Slots);
	GameFree(map->Alloc, map->Keys);
    GameFree(map->Alloc, map->Values);
	map->Capacity = 0;
	map->Count = 0;
}

void 
HashMapKVSet(HashMapKV* map, const void* key, const void* value)
{
	HashMapKVSet_Internal(map, key, value);
}

void 
HashMapKVReplace(HashMapKV* map, const void* key, const void* value)
{
	HashMapKVSetResults res = HashMapKVSet_Internal(map, key, value);
    if (res.Contained)
        memcpy(IndexValue(map, res.Index), value, map->ValueStride);
}

void* 
HashMapKVSetZeroed(HashMapKV* map, const void* key)
{
	HashMapKVSetResults res = HashMapKVSet_Internal(map, key, nullptr);
    return IndexValue(map, res.Index);
}

uint32_t HashMapKVFind(HashMapKV* map, const void* key)
{
	SASSERT(map);
	SASSERT(key);

	if (!map->Slots || map->Count == 0)
		return HASHMAP_NOT_FOUND;

    u64 hash = Hash(key, map->KeyStride);
	u32 idx = ToIdx(hash, map->Capacity);
	u32 probeLength = 0;
	while (true)
	{
		HashKVSlot slot = map->Slots[idx];
		if (!slot.IsUsed || probeLength > slot.ProbeLength)
			return HASHMAP_NOT_FOUND;
		else if (HashMapCompare(map, IndexValue(map, idx), key))
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

void* 
HashMapKVGet(HashMapKV* map, const void* key)
{
	u32 idx = HashMapKVFind(map, key);
    if (idx == HASHMAP_NOT_FOUND)
        return nullptr;
    else
        return IndexValue(map, idx);
}

bool HashMapKVRemove(HashMapKV* map, const void* key)
{
	if (!map->Slots || map->Count == 0)
		return false;

    u64 hash = Hash(key, map->KeyStride);
	u32 idx = ToIdx(hash, map->Capacity);
	while (true)
	{
		HashKVSlot bucket = map->Slots[idx];
		if (!bucket.IsUsed)
		{
			return false;
		}
		else if (HashMapCompare(map, key, IndexKey(map, idx)))
		{
			while (true) // Move any entries after index closer to their ideal probe length.
			{
				uint32_t lastIdx = idx;
				++idx;
				if (idx == map->Capacity)
					idx = 0;

				HashKVSlot nextBucket = map->Slots[idx];
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
	SASSERT_MSG(false, "shouldn't be called");
	return false;
}

void HashMapKVForEach(HashMapKV* map, void(*Fn)(void*, void*, void*), void* stackMemory)
{
	SASSERT(Fn);
	for (uint32_t i = 0; i < map->Capacity; ++i)
	{
		if (map->Slots[i].IsUsed)
		{
			Fn(IndexKey(map, i), IndexValue(map, i), stackMemory);
		}
	}
}

internal HashMapKVSetResults
HashMapKVSet_Internal(HashMapKV* map, const void* key, const void* value)
{
	SASSERT(map);
	SASSERT(key);
	SASSERT(IsAllocatorValid(map->Alloc));
	SASSERT(map->KeyStride > 0);
	SASSERT(map->ValueStride > 0);
	SASSERT(map->KeyStride + map->ValueStride < Kilobytes(16));

	if (map->Count >= map->MaxCount)
	{
		HashMapKVReserve(map, map->Capacity * DEFAULT_RESIZE);
	}

	SASSERT(map->Slots);

    HashKVSlot slot = {};
    void* swapKey = alloca(map->KeyStride);
    SASSERT(swapKey);

    memcpy(swapKey, key, map->KeyStride);

    void* swapValue = alloca(map->ValueStride);
	SASSERT(swapValue);

    if (value)
        memcpy(swapValue, value, map->ValueStride);
    else
        memset(swapValue, 0, map->ValueStride);

	HashMapKVSetResults res;
    res.Index = HASHMAP_NOT_FOUND;
	res.Contained = false;

	u64 hash = Hash(key, map->KeyStride);
	u32 idx = ToIdx(hash, map->Capacity);
	u32 probeLength = 0;
	while (true)
	{
		HashKVSlot bucket = map->Slots[idx];
		if (!bucket.IsUsed) // Bucket is not used
		{
            bucket.IsUsed = true;
            bucket.ProbeLength = probeLength;
            memcpy(IndexKey(map, idx), swapKey, map->KeyStride);
			memcpy(IndexValue(map, idx), swapValue, map->ValueStride);

			++map->Count;

			if (res.Index == HASHMAP_NOT_FOUND)
				res.Index = idx;

			return res;
		}
		else
		{
			if (HashMapCompare(map, IndexKey(map, idx), swapKey))
				return { idx, true };

			if (probeLength > bucket.ProbeLength)
			{
                if (res.Index == HASHMAP_NOT_FOUND)
                    res.Index = idx;

				Swap(bucket.ProbeLength, probeLength, uint32_t);
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

			SASSERT_MSG(probeLength >= 127, "probeLength is >= 127, using bad hash?");
		}
	}
	return res;
}
