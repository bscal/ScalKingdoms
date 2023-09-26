#include "JobsWin32.h"

#include <Windows.h>

#include <assert.h>

void InitThread(void* handle, unsigned int threadID)
{
	assert(handle);
	// Do Windows-specific thread setup:
	HANDLE winHandle = (HANDLE)handle;

	// Put each thread on to dedicated core:
	DWORD_PTR affinityMask = 1ull << threadID;
	DWORD_PTR affinity_result = SetThreadAffinityMask(winHandle, affinityMask);
	assert(affinity_result > 0);

	//// Increase thread priority:
	//BOOL priority_result = SetThreadPriority(handle, THREAD_PRIORITY_HIGHEST);
	//assert(priority_result != 0);
	// Name the thread:
	WCHAR buffer[31] = {};
	wsprintfW(buffer, L"Job_%d", threadID);
	HRESULT hr = SetThreadDescription(winHandle, buffer);
	assert(SUCCEEDED(hr));
}
