#include "HashMapStr.h"

#include "Utils.h"

constexpr global_var u32 DEFAULT_CAPACITY = 2;
constexpr global_var u32 DEFAULT_RESIZE = 2;
constexpr global_var float DEFAULT_LOADFACTOR = .85f;

#define ToIdx(hash, cap) ((u32)((hash) & (u64)((cap) - 1)))
#define IndexArray(arr, idx, stride) (((u8*)(arr)) + ((idx) * (stride)))
#define IndexValue(hashmap, idx) (IndexArray(hashmap->Values, idx, hashmap->ValueStride))

struct HashStrSlot
{
	zpl_string String;
	u32 Hash;
	u16 ProbeLength;
	bool IsUsed;
};

struct HashMapStrSetResults
{
	u32 Index;
	bool Contained;
};

internal HashMapStrSetResults
HashMapStrSet_Internal(HashMapStr* map, const zpl_string key, const void* value);

void 
HashMapStrInitialize(HashMapStr* map, u32 valueStride, u32 capacity, Allocator alloc)
{
	SASSERT(map);
	SASSERT(IsAllocatorValid(alloc));
	SASSERT(valueStride > 0);

	map->Alloc = alloc;
	map->ValueStride = valueStride;
	if (capacity > 0)
		HashMapStrReserve(map, capacity);
}

void 
HashMapStrReserve(HashMapStr* map, uint32_t capacity)
{
	SASSERT(map);
	SASSERT(IsAllocatorValid(map->Alloc));
	SASSERT(map->ValueStride > 0);

	if (capacity == 0)
		capacity = DEFAULT_CAPACITY;

	if (capacity <= map->Capacity)
		return;

	if (!IsPowerOf2_32(capacity))
		capacity = AlignPowTwo32(capacity);

	if (map->Slots)
	{
		HashMapStr tmpMap = {};
		tmpMap.ValueStride = map->ValueStride;
		tmpMap.Alloc = map->Alloc;
		HashMapStrReserve(&tmpMap, capacity);

		for (u32 i = 0; i < map->Capacity; ++i)
		{
			if (map->Slots[i].IsUsed)
			{
				HashMapStrSet(&tmpMap, map->Slots[i].String, IndexValue(map, i));
			}
		}

		GameFree(map->Alloc, map->Slots);
		GameFree(map->Alloc, map->Values);

		SASSERT(map->Count == tmpMap.Count);
		*map = tmpMap;
	}
	else
	{
		map->Capacity = capacity;
		map->MaxCount = (u32)((float)map->Capacity * DEFAULT_LOADFACTOR);

		map->Slots = (HashStrSlot*)GameMalloc(map->Alloc, sizeof(HashStrSlot) * map->Capacity);
		memset(map->Slots, 0, sizeof(HashStrSlot) * map->Capacity);

		map->Values = GameMalloc(map->Alloc, map->ValueStride * map->Capacity);
	}
}

void HashMapStrClear(HashMapStr* map)
{
	memset(map->Slots, 0, sizeof(HashStrSlot) * map->Capacity);
	map->Count = 0;
}

void HashMapStrFree(HashMapStr* map)
{
	GameFree(map->Alloc, map->Slots);
	GameFree(map->Alloc, map->Values);
	map->Capacity = 0;
	map->Count = 0;
}

void
HashMapStrSet(HashMapStr* map, const zpl_string key, const void* value)
{
	HashMapStrSet_Internal(map, key, value);
}

void
HashMapStrReplace(HashMapStr* map, const zpl_string key, const void* value)
{
	HashMapStrSetResults res = HashMapStrSet_Internal(map, key, value);
	if (res.Contained)
		memcpy(IndexValue(map, res.Index), value, map->ValueStride);
}

void*
HashMapStrSetZeroed(HashMapStr* map, const zpl_string key)
{
	HashMapStrSetResults res = HashMapStrSet_Internal(map, key, nullptr);
	return IndexValue(map, res.Index);
}

uint32_t HashMapStrFind(HashMapStr* map, const zpl_string key)
{
	SASSERT(map);
	SASSERT(key);
	SASSERT(IsAllocatorValid(map->Alloc));

	if (!map->Slots || map->Count == 0)
		return HASHMAPSTR_NOT_FOUND;

	size_t length = zpl_string_length(key);
	u32 hash = zpl_fnv32a(key, length);
	u32 idx = hash & (map->Capacity - 1);
	u32 probeLength = 0;
	while (true)
	{
		HashStrSlot slot = map->Slots[idx];
		if (!slot.IsUsed || probeLength > slot.ProbeLength)
			return HASHMAPSTR_NOT_FOUND;
		else if (slot.Hash == hash && zpl_string_are_equal(key, slot.String))
			return idx;
		else
		{
			++probeLength;
			++idx;
			if (idx == map->Capacity)
				idx = 0;
		}
	}
	return HASHMAPSTR_NOT_FOUND;
}

void*
HashMapStrGet(HashMapStr* map, const zpl_string key)
{
	u32 idx = HashMapStrFind(map, key);
	if (idx == HASHMAPSTR_NOT_FOUND)
		return nullptr;
	else
		return IndexValue(map, idx);
}

bool HashMapStrRemove(HashMapStr* map, const zpl_string key)
{
	SASSERT(map);
	SASSERT(key);
	SASSERT(IsAllocatorValid(map->Alloc));

	if (!map->Slots || map->Count == 0)
		return false;

	size_t length = zpl_string_length(key);
	u32 hash = zpl_fnv32a(key, length);
	u32 idx = hash & (map->Capacity - 1);
	while (true)
	{
		HashStrSlot slot = map->Slots[idx];
		if (!slot.IsUsed)
		{
			return false;
		}
		else if (slot.Hash == hash && zpl_string_are_equal(key, slot.String))
		{
			while (true) // Move any entries after index closer to their ideal probe length.
			{
				uint32_t lastIdx = idx;
				++idx;
				if (idx == map->Capacity)
					idx = 0;

				HashStrSlot nextSlot = map->Slots[idx];
				if (!nextSlot.IsUsed || nextSlot.ProbeLength == 0) // No more entires to move
				{
					map->Slots[lastIdx].ProbeLength = 0;
					map->Slots[lastIdx].IsUsed = 0;
					--map->Count;
					return true;
				}
				else
				{
					map->Slots[lastIdx].String = nextSlot.String;
					map->Slots[lastIdx].Hash = nextSlot.Hash;
					map->Slots[lastIdx].ProbeLength = nextSlot.ProbeLength - 1;
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

internal HashMapStrSetResults
HashMapStrSet_Internal(HashMapStr* map, const zpl_string key, const void* value)
{
	SASSERT(map);
	SASSERT(key);
	SASSERT(IsAllocatorValid(map->Alloc));
	SASSERT(map->ValueStride > 0);

	if (map->Count >= map->MaxCount)
	{
		HashMapStrReserve(map, map->Capacity * DEFAULT_RESIZE);
	}

	SASSERT(map->Slots);

	HashStrSlot swapSlot = {};
	swapSlot.String = key;
	swapSlot.IsUsed = true;

	SASSERT(map->ValueStride < Kilobytes(16));
	void* swapValue = _alloca(map->ValueStride);
	SASSERT(swapValue);

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

	size_t length = zpl_string_length(key);
	u32 hash = zpl_fnv32a(key, length);
	swapSlot.Hash = hash;
	u32 idx = hash & (map->Capacity - 1);
	u32 probeLength = 0;
	while (true)
	{
		HashStrSlot* slot = &map->Slots[idx];
		if (!slot->IsUsed) // Bucket is not used
		{
			*slot = swapSlot;
			memcpy(IndexValue(map, idx), swapValue, map->ValueStride);

			++map->Count;

			if (res.Index == HASHMAPSTR_NOT_FOUND)
				res.Index = idx;

			return res;
		}
		else
		{
			if (slot->Hash == hash && zpl_string_are_equal(key, slot->String))
				return { idx, true };

			if (probeLength > slot->ProbeLength)
			{
				if (res.Index == HASHMAPSTR_NOT_FOUND)
					res.Index = idx;

				swapSlot.ProbeLength = probeLength;
				probeLength = slot->ProbeLength;
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

			SASSERT_MSG(probeLength <= 127, "probeLength is > 127, using bad hash?");
		}
	}
	return res;
}
