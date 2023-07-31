#pragma once

#include <stdint.h>
#include <stdlib.h>

#define HashMapAllocate(size, oldSize) _aligned_malloc(size, 16);
#define HashMapFree(ptr, oldSize) _aligned_free(ptr);

#define KeyToIdx(hash, cap) (hash % cap)
#define HashMapSwap(V0, V1, T) T tmp = V0; V0 = V1; V1 = tmp

constexpr static uint32_t HASHMAP_NOT_FOUND = UINT32_MAX;
constexpr static uint32_t HASHMAP_DEFAULT_CAPACITY = 2;
constexpr static uint32_t HASHMAP_DEFAULT_RESIZE = 2;
constexpr static float HASHMAP_LOAD_FACTOR = 0.85f;

struct HashBucket
{
	uint64_t Hash;
	uint32_t ProbeLength;
	bool Tombstoned;
};

template<typename V>
struct HashMap
{
	HashBucket* Keys;
	V* Values;
	uint32_t Capacity;
	uint32_t Count;

	void Reserve(uint32_t capacity)
	{
		if (capacity == 0)
			capacity = HASHMAP_DEFAULT_CAPACITY;
		
		if (capacity < Capacity)
			return;

		if (Keys)
		{
			HashMap<V> tmpMap = {};
			tmpMap.Reserve(capacity);

			for (uint32_t i = 0; i < Capacity; ++i)
			{
				if (!Keys[i].Tombstoned)
				{
					tmpMap.Put(Keys[i].Hash, &Values[i]);
				}
			}

			Free();

			*this = tmpMap;
		}
		else
		{
			Keys = (HashBucket*)HashMapAllocate(sizeof(HashBucket) * capacity, sizeof(HashBucket) * Capacity);
			Values = (V*)HashMapAllocate(sizeof(V) * capacity, sizeof(V) * Capacity);
			Capacity = capacity;

			for (uint32_t i = 0; i < Capacity; ++i)
			{
				Keys[i].ProbeLength = 0;
				Keys[i].Tombstoned = true;
			}
		}
	}
	
	void Free()
	{
		HashMapFree(Keys, sizeof(HashBucket) * Capacity);
		HashMapFree(Values, sizeof(V) * Capacity);
	}

	uint32_t Put(uint64_t hash, const V* value)
	{
		if (Count >= (int)((float)Capacity * HASHMAP_LOAD_FACTOR))
		{
			Reserve(Capacity * HASHMAP_DEFAULT_RESIZE);
		}

		uint64_t swapHash = hash;
		V swapValue = *value;

		uint32_t idx = KeyToIdx(hash, Capacity);
		uint32_t probeLength = 0;
		while (true)
		{
			HashBucket* bucket = &Keys[idx];
			if (bucket->Tombstoned) // Bucket is not used
			{
				bucket->Hash = swapHash;
				bucket->ProbeLength = probeLength;
				bucket->Tombstoned = false;
				Values[idx] = swapValue;
				++Count;
				return idx;
			}
			else
			{
				if (bucket->Hash == hash)
					return idx;

				if (probeLength > bucket->ProbeLength)
				{
					{
						HashMapSwap(bucket->ProbeLength, probeLength, uint32_t);
					}
					{
						HashMapSwap(bucket->Hash, swapHash, uint64_t);
					}
					{
						HashMapSwap(Values[idx], swapValue, V);
					}
				}

				++probeLength;
				++idx;
				if (idx == Capacity)
					idx = 0;
			}
		}
		return UINT32_MAX;
	}

	V* Get(uint64_t hash)
	{
		if (!Keys)
			return nullptr;

		uint32_t probeLength = 0;
		uint32_t idx = KeyToIdx(hash, Capacity);
		while (true)
		{
			HashBucket* bucket = &Keys[idx];
			if (bucket->Tombstoned || probeLength > bucket->ProbeLength)
				return nullptr;
			else if (hash == bucket->Hash)
				return &Values[idx];
			else
			{
				++probeLength;
				++idx;
				if (idx == Capacity)
					idx = 0;
			}
		}
		return nullptr;
	}

	uint32_t Index(uint64_t Hash)
	{
		if (!Keys)
			return nullptr;

		uint32_t probeLength = 0;
		uint32_t idx = KeyToIdx(hash, Capacity);
		while (true)
		{
			HashBucket* bucket = &Keys[idx];
			if (bucket->Tombstoned || probeLength > bucket->ProbeLength)
				return UINT32_MAX;
			else if (hash == bucket->Hash)
				return idx;
			else
			{
				++probeLength;
				++idx;
				if (idx == Capacity)
					idx = 0;
			}
		}
		return UINT32_MAX;
	}

	bool Remove(uint64_t hash)
	{
		uint32_t idx = KeyToIdx(hash, Capacity);
		while (true)
		{
			HashBucket* bucket = &Keys[idx];
			if (bucket->Tombstoned)
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
						if (nextBucket->Tombstoned || nextBucket->ProbeLength == 0) // No more entires to move
						{
							Keys[lastIdx].ProbeLength = 0;
							Keys[lastIdx].Tombstoned = true;
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
};
