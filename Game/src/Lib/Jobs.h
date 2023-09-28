#pragma once

// https://github.com/turanszkij/WickedEngine

#include "Core.h"

void JobsInitialize(u32 maxThreadCount);

struct JobArgs
{
	void* StackMemory;		// stack memory shared within the current group (jobs within a group execute serially)
	u32 JobIndex;			// job index relative to dispatch (like SV_DispatchThreadID in HLSL)
	u32 GroupId;			// group index relative to dispatch (like SV_GroupID in HLSL)
	u32 GroupIndex;			// job index relative to group (like SV_GroupIndex in HLSL)
	bool IsFirstJobInGroup;	// is the current job the first one in the group?
	bool IsLastJobInGroup;	// is the current job the last one in the group?
};

u32 JobsGetThreadCount();

// Defines a state of execution, can be waited on
struct JobHandle
{
	zpl_atomic32 Counter;
};

typedef void(*JobWorkFunc)(JobArgs* args);

// Add a task to execute asynchronously. Any idle thread will execute this.
void JobsExecute(JobHandle* handle, JobWorkFunc task, void* stack);

// Divide a task onto multiple jobs and execute in parallel.
//	jobCount	: how many jobs to generate for this task.
//	groupSize	: how many jobs to execute per thread. Jobs inside a group execute serially. It might be worth to increase for small jobs
//	task		: receives a JobArgs as parameter
void JobsDispatch(JobHandle* handle, u32 jobCount, u32 groupSize, JobWorkFunc task, void* stack);

// Returns the amount of job groups that will be created for a set number of jobs and group size
u32 JobsDispatchGroupCount(u32 jobCount, u32 groupSize);

// Check if any threads are working currently or not
_FORCE_INLINE_ bool
JobHandleIsBusy(const JobHandle* handle)
{
	// Whenever the JobHandle label is greater than zero, it means that there is still work that needs to be done
	return zpl_atomic32_load(&handle->Counter) > 0;
}

// Wait until all threads become idle
// Current thread will become a worker thread, executing jobs
void JobHandleWait(const JobHandle* handle);