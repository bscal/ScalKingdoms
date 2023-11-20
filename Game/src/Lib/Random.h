#pragma once

#include "Base.h"

static_assert(sizeof(int64_t) == 8, "Only usable with 64 bit");

/*  Written in 2016-2018 by David Blackman and Sebastiano Vigna (vigna@acm.org)

To the extent possible under law, the author has dedicated all copyright
and related and neighboring rights to this software to the public domain
worldwide. This software is distributed without any warranty.

See <http://creativecommons.org/publicdomain/zero/1.0/>. */

/// <summary>
/// Xoroshiro256** implementation
/// </summary>
struct SRandom
{
	uint64_t Seed0;
	uint64_t Seed1;
	uint64_t Seed2;
	uint64_t Seed3;
};

SRandom* GetThreadSRandom();

void SRandomInitialize(SRandom* state, uint64_t seed);

uint64_t SRandNext(SRandom* state);

int SRandNextInt(SRandom* state);

/// Value between 0-1
double SRandNextDouble(SRandom* state);

/// Value between 0-1
float SRandNextFloat(SRandom* state);

bool SRandNextBool(SRandom* state);

float SRandNextFloatRange(SRandom* state, float lower, float upper);

/// <summary>
///  lower and upper are inclusive
/// </summary>
uint64_t SRandNextRange(SRandom* state, uint64_t lower, uint64_t upper);

/// <summary>
///  lower and upper are inclusive
/// </summary>
int64_t SRandNextRangeSigned(SRandom* state, int64_t lower, int64_t upper);

/// <summary>
/// Jumps the state, equivalent to 2^128 calls to next,
/// generates 2^128 starting points
/// </summary>
void SRandJump(SRandom* randomState);

/// <summary>
/// Jumps the state, equivalent to 2^192 calls to next,
/// generates 2^64 starting points
/// /// </summary>
void SRandLongJump(SRandom* randomState);



// #################
// # Xoroshiro128+ #
// #################

/// <summary>
/// Xoroshiro128+ generator, Fast and small generator for floats
/// </summary>
struct X128PlusRandom
{
	uint64_t Seed[2];
};

void X128PlusInitialize(X128PlusRandom* state, uint64_t seed);

internal uint64_t X128PlusNext(X128PlusRandom* state);

/// <summary>
/// Value between 0-1
/// </summary>
float X128PlusNextFloat(X128PlusRandom* state);

/* This is the jump function for the generator. It is equivalent
   to 2^64 calls to next(); it can be used to generate 2^64
   non-overlapping subsequences for parallel computations. */
void X128PlusJump(X128PlusRandom* state);

/* This is the long-jump function for the generator. It is equivalent to
   2^96 calls to next(); it can be used to generate 2^32 starting points,
   from each of which jump() will generate 2^32 non-overlapping
   subsequences for parallel distributed computations. */
void X128PlusLongJump(X128PlusRandom* state);