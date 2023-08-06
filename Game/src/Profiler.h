#pragma once

#include "Core.h"

#define ENABLE_PROFILING 0
#define USE_THREADS 0

#if ENABLE_PROFILING
#include <spall/spall.h>

void SpallBegin(const char* name, int len, double time);
void SpallEnd(double time);

struct ProfileZone
{
	ProfileZone(const char* name, int len)
	{
		double time = GetMicroTime();
		SpallBegin(name, len, time);
	}

	~ProfileZone()
	{
		double time = GetMicroTime();
		SpallEnd(time);
	}
};

#define ZONE_BEGIN() ProfileZone(__FUNCTION__, sizeof(__FUNCTION__) - 1)
#define ZONE_BEGIN_EX(str) ProfileZone(str, sizeof(str) - 1)

#else

#define ZONE_BEGIN()
#define ZONE_BEGIN_EX(str)

#endif

void InitProfile(const char* filename);
void ExitProfile();
