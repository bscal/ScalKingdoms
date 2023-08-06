#pragma once

#include "Core.h"

struct BigFlags8
{
	constexpr static int FLAG_COUNT = 8;

	uint8_t Flags;

	_FORCE_INLINE_ bool Get(uint8_t index) const { SASSERT(index < FLAG_COUNT); return BitGet(Flags, index); }
	_FORCE_INLINE_ bool Mask(uint8_t mask) const { return FlagTrue(Flags, mask); }
	_FORCE_INLINE_ void Toggle(uint8_t index) { SASSERT(index < FLAG_COUNT);  Flags = BitToggle(Flags, index); }
	_FORCE_INLINE_ void True(uint8_t index) { SASSERT(index < FLAG_COUNT);  Flags = BitSet(Flags, index); }
	_FORCE_INLINE_ void False(uint8_t index) { SASSERT(index < FLAG_COUNT);  Flags = BitClear(Flags, index); }
	_FORCE_INLINE_ void Set(uint64_t index, bool value)
	{
		SASSERT(index < FLAG_COUNT);
		Flags = (value) ? BitSet(Flags, index) : BitClear(Flags, index);
	}
};

struct BigFlags32
{
	constexpr static int FLAG_COUNT = 32;

	uint8_t Flags;

	_FORCE_INLINE_ bool Get(uint8_t index) const { SASSERT(index < FLAG_COUNT); return BitGet(Flags, index); }
	_FORCE_INLINE_ bool Mask(uint8_t mask) const { return FlagTrue(Flags, mask); }
	_FORCE_INLINE_ void Toggle(uint8_t index) { SASSERT(index < FLAG_COUNT);  Flags = BitToggle(Flags, index); }
	_FORCE_INLINE_ void True(uint8_t index) { SASSERT(index < FLAG_COUNT);  Flags = BitSet(Flags, index); }
	_FORCE_INLINE_ void False(uint8_t index) { SASSERT(index < FLAG_COUNT);  Flags = BitClear(Flags, index); }
	_FORCE_INLINE_ void Set(uint64_t index, bool value)
	{
		SASSERT(index < FLAG_COUNT);
		Flags = (value) ? BitSet(Flags, index) : BitClear(Flags, index);
	}
};

struct BitFlags64
{
	constexpr static int FLAG_COUNT = 64;

	uint64_t Flags;

	_FORCE_INLINE_ bool Get(uint64_t index) const { SASSERT(index < FLAG_COUNT); return BitGet(Flags, index); }
	_FORCE_INLINE_ bool Mask(uint64_t mask) const { return FlagTrue(Flags, mask); }
	_FORCE_INLINE_ void Toggle(uint64_t index) { SASSERT(index < FLAG_COUNT);  Flags = BitToggle(Flags, index); }
	_FORCE_INLINE_ void True(uint8_t index) { SASSERT(index < FLAG_COUNT);  Flags = BitSet(Flags, index); }
	_FORCE_INLINE_ void False(uint8_t index) { SASSERT(index < FLAG_COUNT);  Flags = BitClear(Flags, index); }
	_FORCE_INLINE_ void Set(uint64_t index, bool value)
	{
		SASSERT(index < FLAG_COUNT);
		Flags = (value) ? BitSet(Flags, index) : BitClear(Flags, index);
	}
};

constexpr static uint64_t
CeilBitsToU64(uint64_t v)
{
	float num = (float)v / 64.0f;
	return ((uint64_t)num == num) ? (uint64_t)num : (uint64_t)num + ((num > 0) ? 1 : 0);
}

template<uint64_t SizeInBits>
struct BitArray
{
	constexpr static uint64_t ElementCount = CeilBitsToU64(SizeInBits);

	uint64_t Memory[ElementCount];

	_FORCE_INLINE_ bool Get(uint64_t bit) const
	{
		SASSERT(bit < SizeInBits);
		uint64_t index = bit / 64;
		uint64_t indexBit = bit % 64;
		return BitGet(Memory[index], indexBit);
	}

	_FORCE_INLINE_ void Set(uint64_t bit)
	{
		SASSERT(bit < SizeInBits);
		uint64_t index = bit / 64;
		uint64_t indexBit = bit % 64;
		Memory[index] = BitSet(Memory[index], indexBit);
	}

	_FORCE_INLINE_ void Clear(uint64_t bit)
	{
		SASSERT(bit < SizeInBits);
		uint64_t index = bit / 64;
		uint64_t indexBit = bit % 64;
		Memory[index] = BitClear(Memory[index], indexBit);
	}

	_FORCE_INLINE_ void Toggle(uint64_t bit)
	{
		SASSERT(bit < SizeInBits);
		uint64_t index = bit / 64;
		uint64_t indexBit = bit % 64;
		Memory[index] = BitToggle(Memory[index], indexBit);
	}
};
