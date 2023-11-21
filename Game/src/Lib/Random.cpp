#include "Random.h"

#include "Core.h"

#include <time.h>

thread_local internal_var SRandom GlobalRandom;
thread_local internal_var bool IsInitialized;

SRandom* GetThreadSRandom()
{
	if (!IsInitialized)
	{
		IsInitialized = true;
		time_t t = time(NULL);
		SRandomInitialize(&GlobalRandom, (uint64_t)t);
	}
	return &GlobalRandom;
}

internal uint64_t SplitMixNext64(uint64_t val)
{
	uint64_t z = val + 0x9e3779b97f4a7c15;
	z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
	z = (z ^ (z >> 27)) * 0x94d049bb133111eb;
	return z ^ (z >> 31);
}

internal constexpr float FloatFromBits(uint32_t i)
{
	return (i >> 8) * 0x1.0p-24f;
}

internal constexpr double DoubleFromBits(uint64_t i)
{
	return (i >> 11) * 0x1.0p-53;
}

internal constexpr uint64_t Rotl(const uint64_t x, int k)
{
	return (x << k) | (x >> (64 - k));
}

internal constexpr uint64_t Rotl32(const uint32_t x, int k)
{
	return (x << k) | (x >> (32 - k));
}

// #######################*
// #    Xoroshiro256**    #
// #######################*

void 
SRandomInitialize(SRandom* state, uint64_t seed)
{
	if (!state)
	{
		SError("Xoroshiro128Random state was nullptr");
		SAssert(state);
		return;
	}

	state->Seed0 = seed;
	state->Seed1 = SplitMixNext64(state->Seed0);
	state->Seed2 = SplitMixNext64(state->Seed1);
	state->Seed3 = SplitMixNext64(state->Seed2);
}

uint64_t 
SRandNext(SRandom* state)
{
	SAssert(state);

	#if SCAL_DEBUG
	if (state->Seed0 == 0 && state->Seed1 == 0
		&& state->Seed2 == 0 && state->Seed3 == 0)
	{
		TraceLog(LOG_WARNING, "SRandom state was uninitialized!");
	}
	#endif

	const uint64_t result = Rotl(state->Seed1 * 5, 7) * 9;
	const uint64_t t = state->Seed1 << 17;

	state->Seed2 ^= state->Seed0;
	state->Seed3 ^= state->Seed1;
	state->Seed1 ^= state->Seed2;
	state->Seed0 ^= state->Seed3;

	state->Seed2 ^= t;
	state->Seed3 = Rotl(state->Seed3, 45);
	return result;
}

double SRandNextDouble(SRandom* state)
{
	return DoubleFromBits(SRandNext(state));
}

int SRandNextInt(SRandom* state)
{
	return static_cast<int>((uint32_t)SRandNext(state));
}

float SRandNextFloat(SRandom* state)
{
	return FloatFromBits(SRandNextInt(state));
}

bool SRandNextBool(SRandom* state)
{
	uint64_t randomValue = SRandNext(state);
	return (BitGet(randomValue, 63) == 1);
}

float SRandNextFloatRange(SRandom* state, float lower, float upper)
{
	SAssert(lower < upper);
	float randomValue = SRandNextFloat(state);
	float range = upper - lower;
	return (randomValue * range) + lower;
}

uint64_t SRandNextRange(SRandom* state, uint64_t lower, uint64_t upper)
{
	if (lower > upper)
	{
		SError("[ SRandom ] lower(%d) is > upper(%d)!", lower, upper);
		SAssert(false);
		return 0;
	}
	uint64_t randomValue = SRandNext(state);

	return randomValue % (upper - lower + 1) + lower;
}

int64_t 
SRandNextRangeSigned(SRandom* state, int64_t lower, int64_t upper)
{
	SAssert(state);
	if (lower > upper)
	{
		SError("[ SRandom ] lower(%d) is > upper(%d)!", lower, upper);
		SAssert(false);
		return 0;
	}
	int64_t randomValue = (int64_t)SRandNext(state);
	return randomValue % (upper - lower + 1) + lower;
}

void 
SRandJump(SRandom* state)
{
	SAssert(state);

	constexpr static uint64_t JUMP[] =
	{
		0x180ec6d33cfd0aba,
		0xd5a61266f0c9392c,
		0xa9582618e03fc9aa,
		0x39abdc4529b1661c
	};

	uint64_t s0 = 0;
	uint64_t s1 = 0;
	uint64_t s2 = 0;
	uint64_t s3 = 0;
	for (int i = 0; i < sizeof JUMP / sizeof * JUMP; i++)
	{
		for (int b = 0; b < 64; b++)
		{
			if (JUMP[i] & UINT64_C(1) << b)
			{
				s0 ^= state->Seed0;
				s1 ^= state->Seed1;
				s2 ^= state->Seed2;
				s3 ^= state->Seed3;
			}
			SRandNext(state);
		}
	}
	state->Seed0 = s0;
	state->Seed1 = s1;
	state->Seed2 = s2;
	state->Seed3 = s3;
}

void 
SRandLongJump(SRandom* state)
{
	SAssert(state);

	constexpr static uint64_t LONG_JUMP[] =
	{
		0x76e15d3efefdcbbf,
		0xc5004e441c522fb3,
		0x77710069854ee241,
		0x39109bb02acbe635 };

	uint64_t s0 = 0;
	uint64_t s1 = 0;
	uint64_t s2 = 0;
	uint64_t s3 = 0;
	for (int i = 0; i < sizeof LONG_JUMP / sizeof * LONG_JUMP; i++)
	{
		for (int b = 0; b < 64; b++) {
			if (LONG_JUMP[i] & UINT64_C(1) << b) {
				s0 ^= state->Seed0;
				s1 ^= state->Seed1;
				s2 ^= state->Seed2;
				s3 ^= state->Seed3;
			}
			SRandNext(state);
		}
	}
	state->Seed0 = s0;
	state->Seed1 = s1;
	state->Seed2 = s2;
	state->Seed3 = s3;
}



// #######################
// #    Xoroshiro128+    #
// #######################

void 
X128PlusInitialize(X128PlusRandom* state, uint64_t seed)
{
	if (!state)
	{
		TraceLog(LOG_ERROR, "Xoroshiro128Random state was nullptr");
		return;
	}
	state->Seed[0] = seed;
	state->Seed[1] = SplitMixNext64(seed);
}

internal uint64_t 
X128PlusNext(X128PlusRandom* state)
{
	SAssert(state);

	#if SCAL_DEBUG
	if (state->Seed[0] == 0 && state->Seed[1] == 0)
	{
		TraceLog(LOG_WARNING, "SRandom state was uninitialized!");
	}
	#endif

	const uint64_t s0 = state->Seed[0];
	uint64_t s1 = state->Seed[1];
	const uint64_t result = s0 + s1;

	s1 ^= s0;
	state->Seed[0] = Rotl(s0, 24) ^ s1 ^ (s1 << 16); // a, b
	state->Seed[1] = Rotl(s1, 37); // c

	return result;
}

float X128PlusNextFloat(X128PlusRandom* state)
{
	SAssert(state);
	uint64_t randomValue = X128PlusNext(state);
	uint32_t randomValue32 = uint32_t(randomValue >> 32);
	return FloatFromBits(randomValue32);
}


/* This is the jump function for the generator. It is equivalent
   to 2^64 calls to next(); it can be used to generate 2^64
   non-overlapping subsequences for parallel computations. */
void X128PlusJump(X128PlusRandom* state)
{
	SAssert(state);

	constexpr static uint64_t JUMP[] =
	{ 0xdf900294d8f554a5, 0x170865df4b3201fc };

	uint64_t s0 = 0;
	uint64_t s1 = 0;
	for (int i = 0; i < sizeof JUMP / sizeof * JUMP; i++)
		for (int b = 0; b < 64; b++) {
			if (JUMP[i] & UINT64_C(1) << b) {
				s0 ^= state->Seed[0];
				s1 ^= state->Seed[1];
			}
			X128PlusNext(state);
		}

	state->Seed[0] = s0;
	state->Seed[1] = s1;
}


/* This is the long-jump function for the generator. It is equivalent to
   2^96 calls to next(); it can be used to generate 2^32 starting points,
   from each of which jump() will generate 2^32 non-overlapping
   subsequences for parallel distributed computations. */
void X128PlusLongJump(X128PlusRandom* state)
{
	SAssert(state);

	constexpr static uint64_t LONG_JUMP[] =
	{ 0xd2a98b26625eee7b, 0xdddf9b1090aa7ac1 };

	uint64_t s0 = 0;
	uint64_t s1 = 0;
	for (int i = 0; i < sizeof LONG_JUMP / sizeof * LONG_JUMP; i++)
		for (int b = 0; b < 64; b++) {
			if (LONG_JUMP[i] & UINT64_C(1) << b) {
				s0 ^= state->Seed[0];
				s1 ^= state->Seed[1];
			}
			X128PlusNext(state);
		}

	state->Seed[0] = s0;
	state->Seed[1] = s1;
}