#include "Jobs.h"

#include "Structures/ArrayList.h"

#include <moodycamel/ConcurrentQueue.h>

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
	uint32_t groupID;
	uint32_t groupJobOffset;
	uint32_t groupJobEnd;
};

struct JobQueue
{
	moodycamel::ConcurrentQueue<Job> queue;

	_FORCE_INLINE_ void push_back(const Job& item)
	{
		bool couldEnqueue = queue.try_enqueue(item);
		SAssert(couldEnqueue);
	}

	_FORCE_INLINE_ bool pop_front(Job& item)
	{
		bool notEmpty = queue.try_dequeue(item);
		return notEmpty;
	}
};

// Manages internal state and thread management. Will handle joining and destroying threads
// when finished.
#pragma warning(disable: 4324) // warning for alignments
struct InternalState
{
	uint32_t numCores;
	uint32_t numThreads;
	ArrayList(zpl_thread) threads;
	JobQueue* jobQueuePerThread;
	zpl_atomic32 alive;
	alignas(CACHE_LINE) zpl_atomic32 nextQueue;

	InternalState()
	{
		numCores = 0;
		numThreads = 0;
		jobQueuePerThread = nullptr;
		threads = nullptr;
		alive = {};
		nextQueue = {};
		zpl_atomic32_store(&alive, 1);
		zpl_atomic32_store(&nextQueue, 0);

		SDebugLog("[ Jobs ] Thread state initialized!");
	}

	~InternalState()
	{
		zpl_atomic32_store(&alive, 0); // indicate that new jobs cannot be started from this point

		for (int i = 0; i < ArrayListCount(threads); ++i)
		{
			zpl_semaphore_post(&threads[i].semaphore, 1);
			zpl_thread_join(&threads[i]);
			zpl_thread_destroy(&threads[i]);
		}

		SDebugLog("[ Jobs ] Thread state shutdown!");
	}
} global internal_state;

//	Start working on a job queue
//	After the job queue is finished, it can switch to an other queue and steal jobs from there
internal inline void 
work(uint32_t startingQueue)
{
	for (uint32_t i = 0; i < internal_state.numThreads; ++i)
	{
		Job job;
		JobQueue& job_queue = internal_state.jobQueuePerThread[startingQueue % internal_state.numThreads];
		while (job_queue.pop_front(job))
		{
			JobArgs args;
			args.groupID = job.groupID;
			args.sharedmemory = job.stack;

			for (uint32_t j = job.groupJobOffset; j < job.groupJobEnd; ++j)
			{
				args.jobIndex = j;
				args.groupIndex = j - job.groupJobOffset;
				args.isFirstJobInGroup = (j == job.groupJobOffset);
				args.isLastJobInGroup = (j == job.groupJobEnd - 1);
				job.task(&args);
			}

			zpl_atomic32_fetch_add(&job.handle->counter, -1);
		}
		++startingQueue; // go to next queue
	}
}

void JobsInitialize(uint32_t maxThreadCount)
{
	if (internal_state.numThreads > 0)
	{
		SWarn("Job internal state already initialized");
		return;
	}

	SAssert(maxThreadCount > 0);

	double startTime = GetTime();

	maxThreadCount = std::max(1u, maxThreadCount);

	zpl_affinity affinity;
	zpl_affinity_init(&affinity);

	int coreCount = (int)affinity.core_count;
	int threadCount = (int)affinity.thread_count;

	zpl_affinity_destroy(&affinity);

	internal_state.numCores = coreCount;

	// Determine how many threads to create.
	// - 1 so we don't create a ton of threads on small cpu core sizes
	u32 threadsToCreate = (threadCount <= 4) ? 1 : threadCount - 2;
	// - 1 for main thread
	internal_state.numThreads = std::min(maxThreadCount, std::max(1u, threadsToCreate - 1));

	internal_state.jobQueuePerThread = (JobQueue*)SMalloc(Allocator::Arena, internal_state.numThreads * sizeof(JobQueue));

	// calls constructors for the concurrent queues
	for (u32 i = 0; i < internal_state.numThreads; ++i)
	{
		CALL_CONSTRUCTOR(&internal_state.jobQueuePerThread[i].queue, moodycamel::ConcurrentQueue<Job>);
	}

	ArrayListReserve(Allocator::Arena, internal_state.threads, (int)internal_state.numThreads);

	for (uint32_t threadIdx = 0; threadIdx < internal_state.numThreads; ++threadIdx)
	{
		zpl_thread* thread = &internal_state.threads[threadIdx];
		zpl_thread_init(thread);

		zpl_thread_start_with_stack(thread, [](zpl_thread* thread)
			{
				u32* threadIdx = (u32*)thread->user_data;

				while (zpl_atomic32_load(&internal_state.alive))
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
		, internal_state.numCores
		, threadCount
		, internal_state.numThreads);
	SInfoLog(infoStr);
	
	SAssert(internal_state.numCores > 0);
	SAssert(internal_state.numThreads > 0);
}

uint32_t JobsGetThreadCount()
{
	return internal_state.numThreads;
}

void JobsExecute(JobHandle& handle, JobWorkFunc task, void* stack)
{
	SAssert(task);

	// Context state is updated:
	zpl_atomic32_fetch_add(&handle.counter, 1);

	Job job;
	job.handle = &handle;
	job.task = task;
	job.stack = stack;
	job.groupID = 0;
	job.groupJobOffset = 0;
	job.groupJobEnd = 1;

	zpl_i32 idx = zpl_atomic32_fetch_add(&internal_state.nextQueue, 1) % internal_state.numThreads;
	internal_state.jobQueuePerThread[idx].push_back(job);
	zpl_semaphore_post(&internal_state.threads[idx].semaphore, 1);
}

void JobsDispatch(JobHandle& handle, uint32_t jobCount, uint32_t groupSize, JobWorkFunc task, void* stack)
{
	SAssert(task);
	if (jobCount == 0 || groupSize == 0)
	{
		return;
	}

	uint32_t groupCount = JobsDispatchGroupCount(jobCount, groupSize);

	// Context state is updated:
	zpl_atomic32_fetch_add(&handle.counter, groupCount);

	Job job;
	job.handle = &handle;
	job.task = task;
	job.stack = stack;

	for (uint32_t groupID = 0; groupID < groupCount; ++groupID)
	{
		// For each group, generate one real job:
		job.groupID = groupID;
		job.groupJobOffset = groupID * groupSize;
		job.groupJobEnd = std::min(job.groupJobOffset + groupSize, jobCount);

		zpl_i32 idx = zpl_atomic32_fetch_add(&internal_state.nextQueue, 1) % internal_state.numThreads;
		internal_state.jobQueuePerThread[idx].push_back(job);
		zpl_semaphore_post(&internal_state.threads[idx].semaphore, 1);
	}
}

uint32_t JobsDispatchGroupCount(uint32_t jobCount, uint32_t groupSize)
{
	// Calculate the amount of job groups to dispatch (overestimate, or "ceil"):
	return (jobCount + groupSize - 1) / groupSize;
}

bool JobsIsBusy(JobHandle handle)
{
	// Whenever the JobHandle label is greater than zero, it means that there is still work that needs to be done
	return zpl_atomic32_load(&handle.counter) > 0;
}

void JobsWait(JobHandle handle)
{
	if (JobsIsBusy(handle))
	{
		// Wake any threads that might be sleeping:
		//internal_state.wakeCondition.notify_all();
		for (u32 threadId = 0 ; threadId < internal_state.numThreads; ++threadId)
			zpl_semaphore_post(&internal_state.threads[threadId].semaphore, 1);

		// work() will pick up any jobs that are on stand by and execute them on this thread:
		zpl_i32 idx = zpl_atomic32_fetch_add(&internal_state.nextQueue, 1) % internal_state.numThreads;
		work(idx);

		while (JobsIsBusy(handle))
		{
			// If we are here, then there are still remaining jobs that work() couldn't pick up.
			//	In this case those jobs are not standing by on a queue but currently executing
			//	on other threads, so they cannot be picked up by this thread.
			//	Allow to swap out this thread by OS to not spin endlessly for nothing
			zpl_yield_thread();
		}
	}
}
