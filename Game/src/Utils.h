#pragma once

#include "Core.h"

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

_FORCE_INLINE_ constexpr size_t
AlignSize(size_t size, size_t alignment)
{
	return (size + (alignment - 1)) & ~(alignment - 1);
}

// TODO move
void
TextSplitBuffered(const char* text, char delimiter, int* _RESTRICT_ count, 
	char* _RESTRICT_ buffer, int bufferLength,
	char** _RESTRICT_ splitBuffer, int splitBufferLength);

constexpr void
RemoveWhitespace(char* s)
{
	char* d = s;
	do {
		while (*d == ' ') {
			++d;
		}
	} while (*s++ = *d++);
}

enum STR2INT
{
	STR2INT_SUCCESS,
	STR2INT_OVERFLOW,
	STR2INT_UNDERFLOW,
	STR2INT_INCONVERTIBLE
};

STR2INT Str2Int(int* out, const char* s, int base);

STR2INT Str2UInt(u32* out, const char* s, int base);

int FastAtoi(const char* str);

Color IntToColor(int color);
