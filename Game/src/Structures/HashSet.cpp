#include "HashSet.h"

#include "Utils.h"

#define HashMapKeyIndex(hash, cap) (hash & (cap - 1))
#define HashMapSwap(V0, V1, T) T tmp = V0; V0 = V1; V1 = tmp
#define HashMapKeySize(hashmap) (hashmap->Capacity * sizeof(HashSetBucket))

void HashSet::Initialize(uint32_t capacity, Allocator allocator)
{
	SASSERT(ValidateAllocator(allocator));

	Alloc = allocator;
	if (capacity > 0)
		Reserve(capacity);
}

void HashSet::Reserve(uint32_t capacity)
{
	SASSERT(ValidateAllocator(Alloc));

	if (capacity == 0)
		capacity = HASHSET_DEFAULT_CAPACITY;

	if (capacity <= Capacity)
		return;

	if (!IsPowerOf2_32(capacity))
		capacity = AlignPowTwo32(capacity);

	if (Keys)
	{
		HashSet tmpMap = {};
		tmpMap.Alloc = Alloc;
		tmpMap.Reserve(capacity);

		for (uint32_t i = 0; i < Capacity; ++i)
		{
			if (Keys[i].IsUsed)
			{
				tmpMap.Put(Keys[i].Hash);
			}
		}

		FreeAlign(Alloc, Keys, 16);

		SASSERT(Count == tmpMap.Count);
		*this = tmpMap;
	}
	else
	{
		size_t oldKeysSize = HashMapKeySize(this);

		Capacity = capacity;

		size_t newKeysSize = HashMapKeySize(this);

		Keys = (HashSetBucket*)AllocAlign(Alloc, newKeysSize, 16);

		memset(Keys, 0, sizeof(newKeysSize));
	}
}

void HashSet::Clear()
{
	Count = 0;
	memset(Keys, 0, HashMapKeySize(this));
}

void HashSet::Destroy()
{
	FreeAlign(Alloc, Keys, 16);
	Capacity = 0;
	Count = 0;
}

bool HashSet::Put(uint32_t hash)
{
	if (Count >= (uint32_t)((float)Capacity * HASHSET_LOAD_FACTOR))
	{
		Reserve(Capacity * HASHSET_DEFAULT_RESIZE);
	}

	SASSERT(Keys);

	uint32_t swapHash = hash;
	uint32_t probeLength = 0;

	uint32_t idx = HashMapKeyIndex(hash, Capacity);
	while (true)
	{
		HashSetBucket* bucket = &Keys[idx];
		if (!bucket->IsUsed) // Bucket is not used
		{
			bucket->Hash = swapHash;
			bucket->ProbeLength = probeLength;
			bucket->IsUsed = 1;
			++Count;

			return true;
		}
		else
		{
			if (bucket->Hash == hash)
				break;

			if (probeLength > bucket->ProbeLength)
			{
				{
					HashMapSwap(bucket->ProbeLength, probeLength, uint32_t);
				}
				{
					HashMapSwap(bucket->Hash, swapHash, uint32_t);
				}
			}

			++probeLength;
			++idx;
			if (idx == Capacity)
				idx = 0;
		}
	}
	return false;
}

bool HashSet::Contains(uint32_t hash)
{
	if (!Keys || Count == 0)
		return false;

	uint32_t probeLength = 0;
	uint32_t idx = HashMapKeyIndex(hash, Capacity);
	while (true)
	{
		HashSetBucket bucket = Keys[idx];
		if (!bucket.IsUsed || probeLength > bucket.ProbeLength)
			return false;
		else if (hash == bucket.Hash)
			return true;
		else
		{
			++probeLength;
			++idx;
			if (idx == Capacity)
				idx = 0;
		}
	}
	return false;
}

bool HashSet::Remove(uint32_t hash)
{
	if (!Keys || Count == 0)
		return false;

	uint32_t idx = HashMapKeyIndex(hash, Capacity);
	while (true)
	{
		HashSetBucket* bucket = &Keys[idx];
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

					HashSetBucket* nextBucket = &Keys[idx];
					if (!nextBucket->IsUsed || nextBucket->ProbeLength == 0) // No more entires to move
					{
						Keys[lastIdx].ProbeLength = 0;
						Keys[lastIdx].IsUsed = 0;
						--Count;
						return true;
					}
					else
					{
						Keys[lastIdx].Hash = nextBucket->Hash;
						Keys[lastIdx].ProbeLength = nextBucket->ProbeLength - 1;
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
