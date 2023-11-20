#include "Jobs.h"

#include "Utils.h"
#include "Structures/ArrayList.h"
#include "Structures/QueueThreaded.h"

#ifdef _WIN32
#include "JobsWin32.h"
#endif
#ifdef PLATFORM_LINUX
#include <pthread.h>
#endif

// TODO Should this struct be aligned to cache line?
struct Job
{
	JobWorkFunc task;
	void* stack;
	JobHandle* handle;
	u32 GroupId;
	u32 groupJobOffset;
	u32 groupJobEnd;
};

struct JobQueue
{
	#define JOB_QUEUE_SIZE 128
	QueueThreaded<Job, JOB_QUEUE_SIZE> HighPriorityQueue;
	// TODO possibly implement low prio queue
	//QueueThreaded<Job, JOB_QUEUE_SIZE> LowPriorityQueue;

	_FORCE_INLINE_ void PushBack(const Job& item)
	{
		bool couldEnqueue = HighPriorityQueue.Enqueue(&item);
		SAssert(couldEnqueue);
	}

	_FORCE_INLINE_ bool PopFront(Job& item)
	{
		bool notEmpty = HighPriorityQueue.Dequeue(&item);
		return notEmpty;
	}
};

// Manages internal state and thread management. Will handle joining and destroying threads
// when finished.
struct InternalState
{
	u32 NumCores;
	u32 NumThreads;
	ArrayList(zpl_thread) Threads;
	JobQueue* JobQueuePerThread;
	zpl_atomic32 IsAlive;
	zpl_atomic32 NextQueueIndex;

	InternalState()
	{
		NumCores = 0;
		NumThreads = 0;
		JobQueuePerThread = nullptr;
		Threads = nullptr;
		IsAlive = {};
		NextQueueIndex = {};
		zpl_atomic32_store(&IsAlive, 1);

		SDebugLog("[ Jobs ] Thread state initialized!");
	}

	~InternalState()
	{
		zpl_atomic32_store(&IsAlive, 0); // indicate that new jobs cannot be started from this point

		for (int i = 0; i < ArrayListCount(Threads); ++i)
		{
			zpl_semaphore_post(&Threads[i].semaphore, 1);
			zpl_thread_join(&Threads[i]);
			zpl_thread_destroy(&Threads[i]);
		}

		SDebugLog("[ Jobs ] Thread state shutdown!");
	}
} internal_var JobInternalState;

//	Start working on a job queue
//	After the job queue is finished, it can switch to an other queue and steal jobs from there
internal void 
Work(u32 startingQueue)
{
	for (u32 i = 0; i < JobInternalState.NumThreads; ++i)
	{
		Job job;
		JobQueue* job_queue = &JobInternalState.JobQueuePerThread[startingQueue % JobInternalState.NumThreads];
		while (job_queue->PopFront(job))
		{
			SAssert(job.task);

			for (u32 j = job.groupJobOffset; j < job.groupJobEnd; ++j)
			{
				JobArgs args;
				args.GroupId = job.GroupId;
				args.StackMemory = job.stack;
				args.JobIndex = j;
				args.GroupIndex = j - job.groupJobOffset;
				args.IsFirstJobInGroup = (j == job.groupJobOffset);
				args.IsLastJobInGroup = (j == job.groupJobEnd - 1);
				job.task(&args);
			}
			zpl_atomic32_fetch_add(&job.handle->Counter, -1);
		}
		++startingQueue; // go to next queue
	}
}

void JobsInitialize(u32 maxThreadCount)
{
	if (JobInternalState.NumThreads > 0)
	{
		SWarn("Job internal state already initialized");
		return;
	}

	SAssert(maxThreadCount > 0);

	double startTime = GetTime();

	maxThreadCount = Max(1u, maxThreadCount);

	zpl_affinity affinity;
	zpl_affinity_init(&affinity);

	int coreCount = (int)affinity.core_count;
	int threadCount = (int)affinity.thread_count;

	zpl_affinity_destroy(&affinity);

	JobInternalState.NumCores = coreCount;
	// Uses coreCount instead of threadCount, -1 for main thread
	JobInternalState.NumThreads = (u32)ClampValue(coreCount - 1, 1, (int)maxThreadCount);

	JobInternalState.JobQueuePerThread = (JobQueue*)SCalloc(SAllocatorGeneral(), JobInternalState.NumThreads * sizeof(JobQueue));

	ArrayListReserve(SAllocatorGeneral(), JobInternalState.Threads, (int)JobInternalState.NumThreads);
	SAssert(JobInternalState.Threads);

	for (u32 threadIdx = 0; threadIdx < JobInternalState.NumThreads; ++threadIdx)
	{
		zpl_thread* thread = &JobInternalState.Threads[threadIdx];
		zpl_thread_init(thread);

		zpl_thread_start_with_stack(thread, [](zpl_thread* thread)
			{
				u32* threadIdx = (u32*)thread->user_data;

				while (zpl_atomic32_load(&JobInternalState.IsAlive))
				{
					Work(*threadIdx);

					// Wait for more work
					zpl_semaphore_wait(&thread->semaphore);
				}

				return (zpl_isize)0;
			}, & threadIdx, sizeof(u32));

		// Platform thread
#ifdef _WIN32
		InitThread(thread->win32_handle, threadIdx);
#elif defined(PLATFORM_LINUX)
#define handle_error_en(en, msg) \
               do { errno = en; perror(msg); } while (0)

		int ret;
		cpu_set_t cpuset;
		CPU_ZERO(&cpuset);
		size_t cpusetsize = sizeof(cpuset);

		CPU_SET(threadID, &cpuset);
		ret = pthread_setaffinity_np(worker.native_handle(), cpusetsize, &cpuset);
		if (ret != 0)
			handle_error_en(ret, std::string(" pthread_setaffinity_np[" + std::to_string(threadID) + ']').c_str());

		// Name the thread
		std::string thread_name = "wi::job::" + std::to_string(threadID);
		ret = pthread_setname_np(worker.native_handle(), thread_name.c_str());
		if (ret != 0)
			handle_error_en(ret, std::string(" pthread_setname_np[" + std::to_string(threadID) + ']').c_str());
#undef handle_error_en
#endif
	}

	double timeEnd = GetTime() - startTime;
	const char* infoStr = TextFormat(
		"[ Jobs ] Initialized in [%.3fms]. Cores: %u, Theads: %d. Created %u threads."
		, timeEnd * 1000.0
		, JobInternalState.NumCores
		, threadCount
		, JobInternalState.NumThreads);
	SInfoLog(infoStr);
	
	SAssert(JobInternalState.NumCores > 0);
	SAssert(JobInternalState.NumThreads > 0);
}

u32 JobsGetThreadCount()
{
	return JobInternalState.NumThreads;
}

void JobsExecute(JobHandle* handle, JobWorkFunc task, void* stack)
{
	SAssert(handle);
	SAssert(task);

	// Context state is updated:
	zpl_atomic32_fetch_add(&handle->Counter, 1);

	Job job;
	job.handle = handle;
	job.task = task;
	job.stack = stack;
	job.GroupId = 0;
	job.groupJobOffset = 0;
	job.groupJobEnd = 1;

	zpl_i32 idx = zpl_atomic32_fetch_add(&JobInternalState.NextQueueIndex, 1) % JobInternalState.NumThreads;
	JobInternalState.JobQueuePerThread[idx].PushBack(job);
	zpl_semaphore_post(&JobInternalState.Threads[idx].semaphore, 1);
}

void JobsDispatch(JobHandle* handle, u32 jobCount, u32 groupSize, JobWorkFunc task, void* stack)
{
	SAssert(handle);
	SAssert(task);
	if (jobCount == 0 || groupSize == 0)
	{
		return;
	}

	u32 groupCount = JobsDispatchGroupCount(jobCount, groupSize);

	// Context state is updated:
	zpl_atomic32_fetch_add(&handle->Counter, groupCount);

	Job job;
	job.handle = handle;
	job.task = task;
	job.stack = stack;

	for (u32 GroupId = 0; GroupId < groupCount; ++GroupId)
	{
		// For each group, generate one real job:
		job.GroupId = GroupId;
		job.groupJobOffset = GroupId * groupSize;
		job.groupJobEnd = Min(job.groupJobOffset + groupSize, jobCount);
		
		zpl_i32 idx = zpl_atomic32_fetch_add(&JobInternalState.NextQueueIndex, 1) % JobInternalState.NumThreads;
		JobInternalState.JobQueuePerThread[idx].PushBack(job);
		zpl_semaphore_post(&JobInternalState.Threads[idx].semaphore, 1);
	}
}

u32 JobsDispatchGroupCount(u32 jobCount, u32 groupSize)
{
	// Calculate the amount of job groups to dispatch (overestimate, or "ceil"):
	return (jobCount + groupSize - 1) / groupSize;
}

void JobHandleWait(const JobHandle* handle)
{
	if (JobHandleIsBusy(handle))
	{
		// Wake any threads that might be sleeping:
		//internal_state.wakeCondition.notify_all();
		for (u32 threadId = 0 ; threadId < JobInternalState.NumThreads; ++threadId)
			zpl_semaphore_post(&JobInternalState.Threads[threadId].semaphore, 1);

		// work() will pick up any jobs that are on stand by and execute them on this thread:
		zpl_i32 idx = zpl_atomic32_fetch_add(&JobInternalState.NextQueueIndex, 1) % JobInternalState.NumThreads;
		Work(idx);

		while (JobHandleIsBusy(handle))
		{
			// If we are here, then there are still remaining jobs that work() couldn't pick up.
			//	In this case those jobs are not standing by on a queue but currently executing
			//	on other threads, so they cannot be picked up by this thread.
			//	Allow to swap out this thread by OS to not spin endlessly for nothing
			zpl_yield_thread();
		}
	}
}
