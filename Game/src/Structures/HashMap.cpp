#include "HashMap.h"

#define HashMapKeyIndex(hash, cap) (hash &= (cap - 1))
#define HashMapSwap(V0, V1, T) T tmp = V0; V0 = V1; V1 = tmp
#define HashMapKeySize(hashmap) (hashmap->Capacity * sizeof(HashBucket))
#define HashMapValueSize(hashmap) (hashmap->Capacity * hashmap->Stride)
#define HashMapIdxValue(hashmap, idx) (hashmap->Values[hashmap->Stride * idx])

void HashMap::Initialize(uint32_t stride, uint32_t capacity, HashMapAlloc allocate, HashMapFree free)
{
	SASSERT(stride > 0);
	SASSERT(allocate);
	SASSERT(free);

	Stride = stride;
	Allocate = allocate;
	Free = free;
	if (capacity > 0)
		Reserve(capacity);
}

void HashMap::Reserve(uint32_t capacity)
{
	SASSERT(Allocate);
	SASSERT(Free);
	SASSERT(Stride > 0);

	if (capacity == 0)
		capacity = HASHMAP_DEFAULT_CAPACITY;

	if (capacity <= Capacity)
		return;

	if (!IsPowerOf2_32(capacity))
		capacity = AlignPowTwo32(capacity);

	if (Keys)
	{
		HashMap tmpMap = {};
		tmpMap.Stride = Stride;
		tmpMap.Allocate = Allocate;
		tmpMap.Free = Free;
		tmpMap.Reserve(capacity);

		for (uint32_t i = 0; i < Capacity; ++i)
		{
			if (Keys[i].IsUsed)
			{
				tmpMap.Put(Keys[i].Hash, &Values[i]);
			}
		}

		Free(Keys, HashMapKeySize(this));
		Free(Values, HashMapValueSize(this));

		SASSERT(Count == tmpMap.Count);
		*this = tmpMap;
	}
	else
	{
		size_t oldKeysSize = HashMapKeySize(this);
		size_t oldValuesSize = HashMapValueSize(this);

		Capacity = capacity;

		size_t newKeysSize = HashMapKeySize(this);
		size_t newValuesSize = HashMapValueSize(this);

		Keys = (HashBucket*)Allocate(newKeysSize, oldKeysSize);
		Values = (uint8_t*)Allocate(newValuesSize, oldValuesSize);

		memset(Keys, 0, sizeof(newKeysSize));

		MaxCount = (uint32_t)((float)Capacity * HASHMAP_LOAD_FACTOR);
	}
}

void HashMap::Destroy()
{
	Free(Keys, HashMapKeySize(this));
	Free(Values, HashMapValueSize(this));
	Capacity = 0;
	Count = 0;
}

uint32_t HashMap::Put(uint32_t hash, void* value)
{
	SASSERT(value);

	if (!value)
		return HASHMAP_NOT_FOUND;

	if (Count >= MaxCount)
	{
		Reserve(Capacity * HASHMAP_DEFAULT_RESIZE);
	}

	SASSERT(Keys);

	uint32_t insertedIndex = HASHMAP_NOT_FOUND;
	uint32_t swapHash = hash;
	uint32_t probeLength = 0;

	uint32_t idx = HashMapKeyIndex(hash, Capacity);
	while (true)
	{
		HashBucket* bucket = &Keys[idx];
		if (!bucket->IsUsed) // Bucket is not used
		{
			bucket->Hash = swapHash;
			bucket->ProbeLength = probeLength;
			bucket->IsUsed = 1;
			memcpy(&Values[idx], value, Stride);
			++Count;

			if (insertedIndex == HASHMAP_NOT_FOUND)
				insertedIndex = idx;

			break;
		}
		else
		{
			if (bucket->Hash == hash)
				break;

			if (probeLength > bucket->ProbeLength)
			{
				if (insertedIndex == UINT32_MAX)
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
					uint8_t* currentValue = &Values[idx];
					for (uint32_t i = 0; i < Stride; ++i)
					{
						uint8_t tmp = currentValue[i];
						currentValue[i] = ((uint8_t*)value)[i];
						((uint8_t*)value)[i] = tmp;
					}
				}
			}

			++probeLength;
			++idx;
			if (idx == Capacity)
				idx = 0;
		}
	}

	return insertedIndex;
}

uint32_t HashMap::Index(uint32_t hash)
{
	if (!Keys || Count == 0)
		return HASHMAP_NOT_FOUND;

	uint32_t probeLength = 0;
	uint32_t idx = HashMapKeyIndex(hash, Capacity);
	while (true)
	{
		HashBucket bucket = Keys[idx];
		if (!bucket.IsUsed || probeLength > bucket.ProbeLength)
			return HASHMAP_NOT_FOUND;
		else if (hash == bucket.Hash)
			return idx;
		else
		{
			++probeLength;
			++idx;
			if (idx == Capacity)
				idx = 0;
		}
	}
	return HASHMAP_NOT_FOUND;
}


bool HashMap::Remove(uint64_t hash)
{
	if (!Keys || Count == 0)
		return false;

	uint32_t idx = HashMapKeyIndex(hash, Capacity);
	while (true)
	{
		HashBucket* bucket = &Keys[idx];
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
					if (idx == Capacity)
						idx = 0;

					HashBucket* nextBucket = &Keys[idx];
					if (!nextBucket->IsUsed || nextBucket->ProbeLength == 0) // No more entires to move
					{
						Keys[lastIdx].ProbeLength = 0;
						Keys[lastIdx].IsUsed = true;
						--Count;
						return true;
					}
					else
					{
						Keys[lastIdx].Hash = nextBucket->Hash;
						Keys[lastIdx].ProbeLength = nextBucket->ProbeLength - 1;
						Values[lastIdx] = Values[idx];
					}
				}
			}
			else
			{
				++idx;
				if (idx == Capacity)
					idx = 0; // continue searching till 0 or found equals key
			}
		}
	}
	return false;
}

void HashMap::ForEach(void(*Fn(uint32_t, void*, void*)), void* stackMemory)
{
	SASSERT(Fn);
	for (uint32_t i = 0; i < Capacity; ++i)
	{
		if (Keys[i].IsUsed)
		{
			Fn(Keys[i].Hash, HashMapValuesIndex(this, i, void*), stackMemory);
		}
	}
}
