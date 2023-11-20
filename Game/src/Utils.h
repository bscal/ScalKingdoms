#pragma once

#include "Base.h"

#include <wyhash/wyhash.h>

#include <raylib/src/raylib.h>

template<typename T>
_FORCE_INLINE_ u64
Hash(T& value)
{
	return wyhash(value, sizeof(T), 0, _wyp);
}

#define HashAndMod(value, capacity) (Hash((value)) & (capacity - 1))

_FORCE_INLINE_ double GetMicroTime()
{
	return GetTime() * 1000000.0;
}

_FORCE_INLINE_ constexpr int
IntModNegative(int a, int b)
{
	int res = a % b;
	return (res < 0) ? res + b : res;
}

_FORCE_INLINE_ constexpr size_t
AlignPowTwo64(size_t num)
{
	if (num == 0) return 0;

	size_t power = 1;
	while (num >>= 1) power <<= 1;
	return power;
}

_FORCE_INLINE_ constexpr uint32_t
AlignPowTwo32(uint32_t num)
{
	if (num == 0) return 0;

	uint32_t power = 1;
	while (num >>= 1) power <<= 1;
	return power;
}

_FORCE_INLINE_ constexpr size_t
AlignPowTwo64Ceil(size_t x)
{
	if (x <= 1) return 1;
	size_t power = 2;
	--x;
	while (x >>= 1) power <<= 1;
	return power;
}

_FORCE_INLINE_ constexpr uint32_t
AlignPowTwo32Ceil(uint32_t x)
{
	if (x <= 1) return 1;
	uint32_t power = 2;
	--x;
	while (x >>= 1) power <<= 1;
	return power;
}

_FORCE_INLINE_ constexpr bool
IsPowerOf2_32(uint32_t num)
{
	return (num > 0 && ((num & (num - 1)) == 0));
}

_FORCE_INLINE_ constexpr bool
IsPowerOf2(size_t num)
{
	return (num > 0 && ((num & (num - 1)) == 0));
}

_FORCE_INLINE_ constexpr size_t
AlignSize(size_t size, size_t alignment)
{
	return (size + (alignment - 1)) & ~(alignment - 1);
}

_FORCE_INLINE_ constexpr uint64_t
FNVHash64(const uint8_t* const data, size_t length)
{
	constexpr uint64_t OFFSET_BASIS = 0xcbf29ce484222325;
	constexpr uint64_t PRIME = 0x100000001b3;
	const u8* const castData = (const u8* const)data;
	uint64_t val = OFFSET_BASIS;
	for (size_t i = 0; i < length; ++i)
	{
		val ^= castData[i];
		val *= PRIME;
	}
	return val;
}

_FORCE_INLINE_ constexpr uint32_t
FNVHash32(const void* const data, size_t length)
{
	constexpr uint32_t OFFSET_BASIS = 0x811c9dc5;
	constexpr uint32_t PRIME = 0x1000193;
	const u8* const castData = (const u8* const)data;
	uint32_t val = OFFSET_BASIS;
	for (size_t i = 0; i < length; ++i)
	{
		val ^= castData[i];
		val *= PRIME;
	}
	return val;
}


inline int
FastAtoi(const char* str)
{
	int val = 0;
	for (; *str; ++str)
	{
		val = val * 10 + (*str - '0');
	}
	return val;
}

// TODO move
void
TextSplitBuffered(const char* text, char delimiter, int* _RESTRICT_ count,
				  char* _RESTRICT_ buffer, int bufferLength,
				  char** _RESTRICT_ splitBuffer, int splitBufferLength);

void RemoveWhitespace(char* s);

enum STR2INT
{
	STR2INT_SUCCESS,
	STR2INT_OVERFLOW,
	STR2INT_UNDERFLOW,
	STR2INT_INCONVERTIBLE
};

STR2INT Str2Int(int* out, const char* s, int base);

STR2INT Str2UInt(u32* out, const char* s, int base);

inline Color
IntToColor(int colorInt)
{
	Color c;
	c.r = (i8)(colorInt >> 24);
	c.g = (i8)(colorInt >> 16);
	c.b = (i8)(colorInt >> 8);
	c.a = (i8)colorInt;
	return c;
}

inline int
ColorToInt(Color c)
{
	int res = 0;
	res |= (c.r << 24);
	res |= (c.g << 16);
	res |= (c.b << 8);
	res |= (c.a << 0);
	return res;
}


