#pragma once

// https://github.com/turanszkij/WickedEngine

#include "Core.h"

void JobsInitialize(u32 maxThreadCount = ~0u);

struct JobArgs
{
	u32 jobIndex;			// job index relative to dispatch (like SV_DispatchThreadID in HLSL)
	u32 groupID;			// group index relative to dispatch (like SV_GroupID in HLSL)
	u32 groupIndex;			// job index relative to group (like SV_GroupIndex in HLSL)
	bool isFirstJobInGroup;	// is the current job the first one in the group?
	bool isLastJobInGroup;	// is the current job the last one in the group?
	void* sharedmemory;		// stack memory shared within the current group (jobs within a group execute serially)
};

u32 JobsGetThreadCount();

// Defines a state of execution, can be waited on
struct JobHandle
{
	zpl_atomic32 counter;
};

typedef void(*JobWorkFunc)(const JobArgs* args);

// Add a task to execute asynchronously. Any idle thread will execute this.
void JobsExecute(JobHandle& ctx, JobWorkFunc task, void* stack);

// Divide a task onto multiple jobs and execute in parallel.
//	jobCount	: how many jobs to generate for this task.
//	groupSize	: how many jobs to execute per thread. Jobs inside a group execute serially. It might be worth to increase for small jobs
//	task		: receives a JobArgs as parameter
void JobsDispatch(JobHandle& ctx, u32 jobCount, u32 groupSize, JobWorkFunc task, void* stack);

// Returns the amount of job groups that will be created for a set number of jobs and group size
u32 JobsDispatchGroupCount(u32 jobCount, u32 groupSize);

// Check if any threads are working currently or not
bool JobsIsBusy(JobHandle ctx);

// Wait until all threads become idle
// Current thread will become a worker thread, executing jobs
void JobsWait(JobHandle ctx);