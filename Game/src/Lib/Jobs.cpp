#include "Jobs.h"

#include "Utils.h"
#include "Structures/ArrayList.h"
#include "Structures/QueueThreaded.h"

//#include <moodycamel/ConcurrentQueue.h>

#ifdef _WIN32
#include "JobsWin32.h"
#endif
#ifdef PLATFORM_LINUX
#include <pthread.h>
#endif

struct Job
{
	JobWorkFunc task;
	void* stack;
	JobHandle* handle;
	uint32_t GroupId;
	uint32_t groupJobOffset;
	uint32_t groupJobEnd;
};

struct JobQueue
{
	//moodycamel::ConcurrentQueue<Job> Queue;

	QueueThreaded<Job, 256> Queue2;

	//JobQueue()
	//{
		//Queue = moodycamel::ConcurrentQueue<Job>(256);
	//}

	_FORCE_INLINE_ void PushBack(const Job& item)
	{
		//bool couldEnqueue = Queue.try_enqueue(item);
		bool couldEnqueue = Queue2.Enqueue(&item);
		SAssert(couldEnqueue);
	}

	_FORCE_INLINE_ bool PopFront(Job& item)
	{
		//bool notEmpty = Queue.try_dequeue(item);
		bool notEmpty = Queue2.Dequeue(&item);
		return notEmpty;
	}
};

// Manages internal state and thread management. Will handle joining and destroying threads
// when finished.
#pragma warning(disable: 4324) // warning for alignments
struct InternalState
{
	uint32_t NumCores;
	uint32_t NumThreads;
	ArrayList(zpl_thread) Threads;
	JobQueue* JobQueuePerThread;
	zpl_atomic32 IsAlive;
	alignas(CACHE_LINE) zpl_atomic32 NextQueueIndex;

	InternalState()
	{
		NumCores = 0;
		NumThreads = 0;
		JobQueuePerThread = nullptr;
		Threads = nullptr;
		IsAlive = {};
		NextQueueIndex = {};
		zpl_atomic32_store(&IsAlive, 1);
		zpl_atomic32_store(&NextQueueIndex, 0);

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
} global JobInternalState;

//	Start working on a job queue
//	After the job queue is finished, it can switch to an other queue and steal jobs from there
internal inline void 
work(uint32_t startingQueue)
{
	for (uint32_t i = 0; i < JobInternalState.NumThreads; ++i)
	{
		Job job;
		JobQueue* job_queue = &JobInternalState.JobQueuePerThread[startingQueue % JobInternalState.NumThreads];
		while (job_queue->PopFront(job))
		{
			JobArgs args;
			args.GroupId = job.GroupId;
			args.StackMemory = job.stack;

			for (uint32_t j = job.groupJobOffset; j < job.groupJobEnd; ++j)
			{
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

void JobsInitialize(uint32_t maxThreadCount)
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

	// Determine how many threads to create.
	// - 1 so we don't create a ton of threads on small cpu core sizes
	u32 threadsToCreate = (threadCount <= 4) ? 1 : threadCount - 1;
	// - 1 for main thread
	JobInternalState.NumThreads = (u32)Clamp(threadsToCreate - 1, 1, (int)maxThreadCount);

	JobInternalState.JobQueuePerThread = (JobQueue*)SMalloc(Allocator::Arena, JobInternalState.NumThreads * sizeof(JobQueue));

	// calls constructors for the concurrent queues
	//for (u32 i = 0; i < JobInternalState.NumThreads; ++i)
	//{
		//CALL_CONSTRUCTOR(&JobInternalState.JobQueuePerThread[i], JobQueue);
	//}

	ArrayListReserve(Allocator::Arena, JobInternalState.Threads, (int)JobInternalState.NumThreads);

	for (uint32_t threadIdx = 0; threadIdx < JobInternalState.NumThreads; ++threadIdx)
	{
		zpl_thread* thread = &JobInternalState.Threads[threadIdx];
		zpl_thread_init(thread);

		zpl_thread_start_with_stack(thread, [](zpl_thread* thread)
			{
				u32* threadIdx = (u32*)thread->user_data;

				while (zpl_atomic32_load(&JobInternalState.IsAlive))
				{
					work(*threadIdx);

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
#endif // _WIN32
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

uint32_t JobsGetThreadCount()
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

void JobsDispatch(JobHandle* handle, uint32_t jobCount, uint32_t groupSize, JobWorkFunc task, void* stack)
{
	SAssert(handle);
	SAssert(task);
	if (jobCount == 0 || groupSize == 0)
	{
		return;
	}

	uint32_t groupCount = JobsDispatchGroupCount(jobCount, groupSize);

	// Context state is updated:
	zpl_atomic32_fetch_add(&handle->Counter, groupCount);

	Job job;
	job.handle = handle;
	job.task = task;
	job.stack = stack;

	for (uint32_t GroupId = 0; GroupId < groupCount; ++GroupId)
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

uint32_t JobsDispatchGroupCount(uint32_t jobCount, uint32_t groupSize)
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
		work(idx);

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
