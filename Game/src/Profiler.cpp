#include "Profiler.h"

#if ENABLE_PROFILING

internal_var SpallProfile SpallCtx;

#if USE_THREADS 

struct ThreadedProfiler
{
	SpallBuffer Buffer;
	uint32_t ThreadId;
	double Timer;

	ThreadedProfiler()
	{
		Buffer.length = Megabytes(1);
		Buffer.data = zpl_alloc(zpl_heap_SAllocator(), Buffer.length);
		SAssert(Buffer.data);
		bool spallBufferInit = spall_buffer_init(&SpallCtx, &Buffer);
		SAssert(spallBufferInit);
		if (spallBufferInit)
		{
			ThreadId = zpl_thread_current_id();
		}
		else
		{
			ThreadId = 0;
			zpl_free(zpl_heap_SAllocator(), Buffer.data);
		}
		Timer = 0.0f;
	}

	~ThreadedProfiler()
	{
		spall_buffer_quit(&SpallCtx, &Buffer);
		zpl_free(zpl_heap_SAllocator(), Buffer.data);
	}

};

thread_local ThreadedProfiler SpallData;

#else

internal_var SpallBuffer SpallData;

#endif

void SpallBegin(const char* name, int len, double time)
{
#if USE_THREADS
	spall_buffer_begin_ex(&SpallCtx, &SpallData.Buffer, name, len, time, SpallData.ThreadId, 0);
	double curTime = GetTime();
	double timeDiff = curTime - SpallData.Timer;

	#define TIME_TILL_FLUSH 10.0
	if (timeDiff >= TIME_TILL_FLUSH)
	{
		SpallData.Timer = curTime;
		spall_buffer_flush(&SpallCtx, &SpallData.Buffer);
	}
#else
	spall_buffer_begin(&SpallCtx, &SpallData, name, len, time);
#endif
}

void SpallEnd(double time)
{
#if USE_THREADS
	spall_buffer_end_ex(&SpallCtx, &SpallData.Buffer, time, SpallData.ThreadId, 0);
#else
	spall_buffer_end(&SpallCtx, &SpallData, time);
#endif
}

#endif

void InitProfile(const char* filename)
{
#if SCAL_ENABLE_PROFILING
	SpallCtx = spall_init_file(filename, 1);

#if !USE_THREADS
#define BUFFER_SIZE (64 * 1024 * 1024)
	void* buffer = SAlloc(SSAllocator::Malloc, BUFFER_SIZE, MemoryTag::Profiling);
	memset(buffer, 1, BUFFER_SIZE);

	SpallData = {};
	SpallData.length = BUFFER_SIZE;
	SpallData.data = buffer;

	spall_buffer_init(&SpallCtx, &SpallData);
#endif

	SLOG_INFO("[ PROFILING ] Spall profiling initialized");
#endif
}

void ExitProfile()
{
#if SCAL_ENABLE_PROFILING
#if !USE_THREADS
	spall_buffer_quit(&SpallCtx, &SpallData);
#endif
	spall_quit(&SpallCtx);
#endif
}