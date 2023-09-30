#include "HashSet.h"

#include "Utils.h"

#define HashMapKeyIndex(hash, cap) (hash & (cap - 1))
#define HashMapSwap(V0, V1, T) T tmp = V0; V0 = V1; V1 = tmp
#define HashMapKeySize(hashmap) (hashmap->Capacity * sizeof(HashSetBucket))

void HashSetInitialize(HashSet* set, uint32_t capacity, SAllocator SAllocator)
{
	SAssert(IsAllocatorValid(SAllocator));

	set->Alloc = SAllocator;
	if (capacity > 0)
		HashSetReserve(set, capacity);
}

void HashSetReserve(HashSet* set, uint32_t capacity)
{
	SAssert(IsAllocatorValid(set->Alloc));

	if (capacity == 0)
		capacity = HASHSET_DEFAULT_CAPACITY;

	if (capacity <= set->Capacity)
		return;

	if (!IsPowerOf2_32(capacity))
		capacity = AlignPowTwo32(capacity);

	if (set->Keys)
	{
		HashSet tmpSet = {};
		tmpSet.Alloc = set->Alloc;
		HashSetReserve(&tmpSet, capacity);

		for (uint32_t i = 0; i < set->Capacity; ++i)
		{
			if (set->Keys[i].IsUsed)
			{
				HashSetSet(&tmpSet, set->Keys[i].Hash);
			}
		}

		SFree(set->Alloc, set->Keys);

		SAssert(set->Count == tmpSet.Count);
		*set = tmpSet;
	}
	else
	{
		set->Capacity = capacity;

		size_t newKeysSize = HashMapKeySize(set);

		set->Keys = (HashSetBucket*)SAlloc(set->Alloc, newKeysSize);

		memset(set->Keys, 0, sizeof(newKeysSize));
	}
}

void HashSetClear(HashSet* set)
{
	set->Count = 0;
	memset(set->Keys, 0, HashMapKeySize(set));
}

void HashSetDestroy(HashSet* set)
{
	SFree(set->Alloc, set->Keys);
	set->Capacity = 0;
	set->Count = 0;
}

bool HashSetSet(HashSet* set, uint64_t hash)
{
	if (set->Count >= (uint32_t)((float)set->Capacity * HASHSET_LOAD_FACTOR))
	{
		HashSetReserve(set, set->Capacity * HASHSET_DEFAULT_RESIZE);
	}

	SAssert(set->Keys);

	uint64_t swapHash = hash;
	uint32_t probeLength = 0;

	uint32_t idx = HashMapKeyIndex(hash, set->Capacity);
	while (true)
	{
		HashSetBucket* bucket = &set->Keys[idx];
		if (!bucket->IsUsed) // Bucket is not used
		{
			bucket->Hash = swapHash;
			bucket->ProbeLength = probeLength;
			bucket->IsUsed = 1;
			++set->Count;

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
					HashMapSwap(bucket->Hash, swapHash, uint64_t);
				}
			}

			++probeLength;
			++idx;
			if (idx == set->Capacity)
				idx = 0;
		}
	}
	return false;
}

bool HashSetContains(HashSet* set, uint64_t hash)
{
	if (!set->Keys || set->Count == 0)
		return false;

	uint32_t probeLength = 0;
	uint32_t idx = HashMapKeyIndex(hash, set->Capacity);
	while (true)
	{
		HashSetBucket bucket = set->Keys[idx];
		if (!bucket.IsUsed || probeLength > bucket.ProbeLength)
			return false;
		else if (hash == bucket.Hash)
			return true;
		else
		{
			++probeLength;
			++idx;
			if (idx == set->Capacity)
				idx = 0;
		}
	}
	return false;
}

bool HashSetRemove(HashSet* set, uint64_t hash)
{
	if (!set->Keys || set->Count == 0)
		return false;

	uint32_t idx = HashMapKeyIndex(hash, set->Capacity);
	while (true)
	{
		HashSetBucket* bucket = &set->Keys[idx];
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
					if (idx == set->Capacity)
						idx = 0;

					HashSetBucket* nextBucket = &set->Keys[idx];
					if (!nextBucket->IsUsed || nextBucket->ProbeLength == 0) // No more entires to move
					{
						set->Keys[lastIdx].ProbeLength = 0;
						set->Keys[lastIdx].IsUsed = 0;
						--set->Count;
						return true;
					}
					else
					{
						set->Keys[lastIdx].Hash = nextBucket->Hash;
						set->Keys[lastIdx].ProbeLength = nextBucket->ProbeLength - 1;
					}
				}
			}
			else
			{
				++idx;
				if (idx == set->Capacity)
					idx = 0; // continue searching till 0 or found equals key
			}
		}
	}
	return false;
}
