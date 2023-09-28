#include "Memory.h"

#include "GameState.h"
#include "Structures/HashMapT.h"



#if TRACK_MEMORY
struct MemoryInfo
{
	size_t Size;
	const char* File;
	const char* Function;
	int Line;
	bool IsIgnoringFree;
};

struct MemoryInfoState
{
	HashMapT<void*, MemoryInfo> AllocationMap[(int)Allocator::MaxAllocators];
	const char* AdditionalFile;
	const char* AdditionalFunction;
	int AdditionalLine;
	void* SearchMemoryAddress;
	int IsIgnoringFree;
	bool IsInitialized;
};
internal_var MemoryInfoState MemoryState;

#endif

void InitializeMemoryTracking()
{
#if TRACK_MEMORY
	if (MemoryState.IsInitialized)
	{
		SError("MemoryState already initialized");
	}

	for (int i = 0; i < (int)Allocator::MaxAllocators; ++i)
	{
		if (i == (int)Allocator::Frame || i == (int)Allocator::MallocUntracked)
			continue;

		HashMapTInitialize(&MemoryState.AllocationMap[i], 1024, Allocator::MallocUntracked);
	}
	MemoryState.IsInitialized = true;
#endif
}

void ShutdownMemoryTracking()
{
#if TRACK_MEMORY
	if (!MemoryState.IsInitialized)
	{
		SError("MemoryState not initialized");
	}

	if (MemoryState.IsIgnoringFree != 0)
	{
		SError("MemoryState.IsIgnoreingFree stack value is not 0!");
	}

	for (int i = 0; i < (int)Allocator::MaxAllocators; ++i)
	{
		if (i == (int)Allocator::Frame || i == (int)Allocator::MallocUntracked)
			continue;

		HashMapTForEach<void*, MemoryInfo>(&MemoryState.AllocationMap[i], [](void** key, MemoryInfo* info, void* stack)
			{
				const char* allocatorName = (const char*)stack;
				if (!info->IsIgnoringFree)
				{
					SDebugLog("[ Memory ] Possible memory leak detected.\n \tType: %s\n \tAddress: %p\n \tSize(bytes): %d\n"
						"\tFile: %s\n \tFunction: %s\n \tLine#: %d\n"
						, allocatorName, *key, info->Size, info->File, info->Function, info->Line);
				}
			}, (void*)AllocatorNames[i]);

		HashMapTDestroy(&MemoryState.AllocationMap[i]);
	}
	MemoryState.IsInitialized = false;
#endif
}

void Internal_PushMemoryIgnoreFree()
{
#if TRACK_MEMORY
	++MemoryState.IsIgnoringFree;
#endif
}

void Internal_PopMemoryIgnoreFree()
{
#if TRACK_MEMORY
	if (!MemoryState.IsIgnoringFree)
	{
		SWarn("PopMemoryIgnoreFree when already 0");
	}

	--MemoryState.IsIgnoringFree;
#endif
}

void Internal_PushMemoryAdditionalInfo(const char* file, const char* func, int line)
{
#if TRACK_MEMORY
	MemoryState.AdditionalFile = file;
	MemoryState.AdditionalFunction = func;
	MemoryState.AdditionalLine = line;
#endif
}

void Internal_PopMemoryAdditionalInfo()
{
#if TRACK_MEMORY
	MemoryState.AdditionalFile = nullptr;
	MemoryState.AdditionalFunction = nullptr;
	MemoryState.AdditionalLine = 0;
#endif
}

void Internal_PushMemoryPointer(void* address)
{
#if TRACK_MEMORY
	if (MemoryState.SearchMemoryAddress)
	{
		SError("Already pushed memory pointer!");
	}
	MemoryState.SearchMemoryAddress = address;
#endif
}
void Internal_PopMemoryPointer()
{
#if TRACK_MEMORY
	if (!MemoryState.SearchMemoryAddress)
	{
		SError("No memory pointer to pop!");
	}
	MemoryState.SearchMemoryAddress = nullptr;
#endif
}

inline void*
GameAllocator_Internal(AllocatorAction allocatorType, void* ptr, size_t newSize, size_t oldSize, u32 align,
	const char* file, const char* func, u32 line)
{
	void* res;

	switch (allocatorType)
	{
	case (ALLOCATOR_TYPE_MALLOC):
	{
		res = zpl_alloc_align(GetGameAllocator(), newSize, align);
	} break;

	case (ALLOCATOR_TYPE_REALLOC):
	{
		res = zpl_resize_align(GetGameAllocator(), ptr, oldSize, newSize, align);
	} break;

	case (ALLOCATOR_TYPE_FREE):
	{
		zpl_free(GetGameAllocator(), ptr);
		res = nullptr;
	} break;

	default:
		SError("Invalid allocator type");
		res = nullptr;
	}

#if SCAL_DEBUG
	if (allocatorType != ALLOCATOR_TYPE_FREE)
		SAssert(res);
#endif

#if TRACK_MEMORY
	if (MemoryState.IsInitialized)
	{
		int allocatorIndex = (int)Allocator::Arena;
		if (allocatorType == ALLOCATOR_TYPE_FREE)
		{
			SAssert(ptr);
			HashMapTRemove(&MemoryState.AllocationMap[allocatorIndex], &ptr);
		}
		else
		{
			MemoryInfo* oldInfo = nullptr;
			if (ptr)
			{
				oldInfo = HashMapTGet(&MemoryState.AllocationMap[allocatorIndex], &ptr);
				if (oldInfo)
				{
					HashMapTRemove(&MemoryState.AllocationMap[allocatorIndex], &ptr);
				}
			}
			else if (MemoryState.SearchMemoryAddress)
			{
				oldInfo = HashMapTGet(&MemoryState.AllocationMap[allocatorIndex], &MemoryState.SearchMemoryAddress);
			}
			
			MemoryInfo memInfo = (oldInfo) ? *oldInfo : MemoryInfo{};
			memInfo.Size = newSize;
			if (!oldInfo)
			{
				memInfo.File = file;
				memInfo.Function = func;
				memInfo.Line = line;
				memInfo.IsIgnoringFree = MemoryState.IsIgnoringFree;
			}

			if (MemoryState.AdditionalFile)
			{
				memInfo.File = MemoryState.AdditionalFile;
				memInfo.Function = MemoryState.AdditionalFunction;
				memInfo.Line = MemoryState.AdditionalLine;
			}

			HashMapTSet(&MemoryState.AllocationMap[allocatorIndex], &res, &memInfo);
		}
	}
#endif

	return res;
}

void*
FrameAllocator_Internal(AllocatorAction allocatorType, void* ptr, size_t newSize, size_t oldSize, u32 align,
	const char* file, const char* func, u32 line)
{
	void* res;

	switch (allocatorType)
	{
	case (ALLOCATOR_TYPE_MALLOC):
	{
		res = LinearArenaAlloc(&GetGameState()->FrameMemory, newSize);
	} break;

	case (ALLOCATOR_TYPE_REALLOC):
	{
		res = LinearArenaAlloc(&GetGameState()->FrameMemory, newSize);
		if (ptr)
		{
			SCopy(res, ptr, oldSize);
		}
	} break;

	case (ALLOCATOR_TYPE_FREE):
	{
		// Frame allocator frees at end of frame
		res = nullptr;
	} break;

	default:
		SError("Invalid allocator type");
		res = nullptr;
	}

#if SCAL_DEBUG
	if (allocatorType != ALLOCATOR_TYPE_FREE)
	{
		SAssert(res);
	}
#endif

	return res;
}

inline void*
MallocAllocator_Internal(AllocatorAction allocatorType, void* ptr, size_t newSize, size_t oldSize, u32 align,
	const char* file, const char* func, u32 line)
{
	void* res;

	switch (allocatorType)
	{
	case (ALLOCATOR_TYPE_MALLOC):
	{
		res = zpl_alloc_align(zpl_heap_allocator(), newSize, align);
	} break;

	case (ALLOCATOR_TYPE_REALLOC):
	{
		res = zpl_resize_align(zpl_heap_allocator(), ptr, oldSize, newSize, align);
	} break;

	case (ALLOCATOR_TYPE_FREE):
	{
		zpl_free(zpl_heap_allocator(), ptr);
		res = nullptr;
	} break;

	default:
		SError("Invalid allocator type");
		res = nullptr;
	}

#if SCAL_DEBUG
	if (allocatorType != ALLOCATOR_TYPE_FREE)
		SAssert(res);
#endif

#if TRACK_MEMORY
	if (MemoryState.IsInitialized)
	{
		int allocatorIndex = (int)Allocator::Malloc;
		if (allocatorType == ALLOCATOR_TYPE_FREE)
		{
			SAssert(ptr);
			HashMapTRemove(&MemoryState.AllocationMap[allocatorIndex], &ptr);
		}
		else
		{
			MemoryInfo* oldInfo = nullptr;
			if (ptr)
			{
				oldInfo = HashMapTGet(&MemoryState.AllocationMap[allocatorIndex], &ptr);
				if (allocatorType == ALLOCATOR_TYPE_REALLOC && oldInfo)
				{
					HashMapTRemove(&MemoryState.AllocationMap[allocatorIndex], &ptr);
				}
			}
			else if (MemoryState.SearchMemoryAddress)
			{
				oldInfo = HashMapTGet(&MemoryState.AllocationMap[allocatorIndex], &MemoryState.SearchMemoryAddress);
			}


			MemoryInfo memInfo = (oldInfo) ? *oldInfo : MemoryInfo{};
			memInfo.Size = newSize;
			if (!oldInfo)
			{
				memInfo.File = file;
				memInfo.Function = func;
				memInfo.Line = line;
				memInfo.IsIgnoringFree = MemoryState.IsIgnoringFree;
			}

			if (MemoryState.AdditionalFile)
			{
				memInfo.File = MemoryState.AdditionalFile;
				memInfo.Function = MemoryState.AdditionalFunction;
				memInfo.Line = MemoryState.AdditionalLine;
			}
			HashMapTSet(&MemoryState.AllocationMap[allocatorIndex], &res, &memInfo);
		}
	}
#endif

	return res;
}

void*
MallocUntrackedAllocator_Internal(AllocatorAction allocatorType, void* ptr, size_t newSize, size_t oldSize, u32 align,
	const char* file, const char* func, u32 line)
{
	void* res;

	switch (allocatorType)
	{
	case (ALLOCATOR_TYPE_MALLOC):
	{
		res = zpl_alloc_align(zpl_heap_allocator(), newSize, align);
	} break;

	case (ALLOCATOR_TYPE_REALLOC):
	{
		res = zpl_resize_align(zpl_heap_allocator(), ptr, oldSize, newSize, align);
	} break;

	case (ALLOCATOR_TYPE_FREE):
	{
		zpl_free(zpl_heap_allocator(), ptr);
		res = nullptr;
	} break;

	default:
		SError("Invalid allocator type");
		res = nullptr;
	}

#if SCAL_DEBUG
	if (allocatorType != ALLOCATOR_TYPE_FREE)
		SAssert(res);
#endif

	return res;
}
