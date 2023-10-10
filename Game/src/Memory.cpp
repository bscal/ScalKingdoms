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
	int ResizeTracker;
	bool IsIgnoringFree;
};

struct MemoryInfoState
{
	HashMapT<void*, MemoryInfo> AllocationMap[(int)SAllocatorId::Max];
	const char* AdditionalFile;
	const char* AdditionalFunction;
	int AdditionalLine;
	void* SearchMemoryAddress;
	int IsIgnoringFree;
	bool IsInitialized;
};
internal_var MemoryInfoState MemoryState;

constant_var const char* ALLOCATOR_NAMES[] =
{
	"General Purpose",
	"Frame",
	"Malloc",
	"Arena"
};

#endif

void InitializeMemoryTracking()
{
#if TRACK_MEMORY
	if (MemoryState.IsInitialized)
	{
		SError("MemoryState already initialized");
	}

	for (int i = 0; i < (int)SAllocatorId::Max; ++i)
	{
		if (i == (int)SAllocatorId::Frame)
			continue;

		HashMapTInitialize(&MemoryState.AllocationMap[i], 1024, SAllocatorMalloc());
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

	MemoryState.IsInitialized = false;

	for (int i = 0; i < (int)SAllocatorId::Max; ++i)
	{
		if (i == (int)SAllocatorId::Frame)
			continue;

		HashMapTForEach<void*, MemoryInfo>(&MemoryState.AllocationMap[i], [](void** key, MemoryInfo* info, void* stack)
										   {
											   const char* SAllocatorName = (const char*)stack;
											   if (!info->IsIgnoringFree)
											   {
												   SDebugLog("[ Memory ] Possible memory leak detected.\n \tType: %s\n \tAddress: %p\n \tSize(bytes): %d\n"
															 "\tFile: %s\n \tFunction: %s\n \tLine#: %d\n"
															 , SAllocatorName, *key, info->Size, info->File, info->Function, info->Line);
											   }
										   }, (void*)ALLOCATOR_NAMES[i]);

		//HashMapTDestroy(&MemoryState.AllocationMap[i]);
	}
#endif
}

internal void
HandleMemoryTracking(int allocatorId, SAllocatorType allocatorType, void* oldPtr, size_t oldSize,
					 void* newPtr, size_t newSize, const char* file, const char* function, int line)
{
	if (MemoryState.IsInitialized)
	{
		// Free we just remove from map
		if (allocatorType == ALLOCATOR_TYPE_FREE)
		{
			SAssert(oldPtr);
			HashMapTRemove(&MemoryState.AllocationMap[allocatorId], &oldPtr);
		}
		else
		{
			MemoryInfo memInfo;	// Info to insert
			MemoryInfo* oldInfo = nullptr; // Was any old entry found
			if (oldPtr)
			{
				oldInfo = HashMapTGet(&MemoryState.AllocationMap[allocatorId], &oldPtr);
				if (oldInfo)
				{
					memInfo = *oldInfo;
					// Remove old info
					HashMapTRemove(&MemoryState.AllocationMap[allocatorId], &oldPtr);
				}
			}
			// NOTE: Certain reallocations may not free realloc and instead alloc then free later,
			// in this case we want to search for previous allocations and also not remove any old allocations.
			// HashMap::Resize is a function that does this.
			else if (MemoryState.SearchMemoryAddress)
			{
				oldInfo = HashMapTGet(&MemoryState.AllocationMap[allocatorId], &MemoryState.SearchMemoryAddress);
				// Handles the case where the hashmap for storing allocations resizes
				// since those hashmaps don't exist yet to store their own allocation
				if (!oldInfo) 
					return;
				else
					memInfo = *oldInfo;
			}

			if (!oldInfo)
				memInfo = {};

			memInfo.Size = newSize;
			if (!oldInfo)
			{
				memInfo.File = file;
				memInfo.Function = function;
				memInfo.Line = line;
				memInfo.IsIgnoringFree = MemoryState.IsIgnoringFree;
				memInfo.ResizeTracker = 0;
			}
			else 
			{
				// Already exists just reuse it's data.
				memInfo.ResizeTracker += 1;
			}
			HashMapTSet(&MemoryState.AllocationMap[allocatorId], &newPtr, &memInfo);
		}
	}
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

SAllocatorProc(GameAllocatorProc)
{
	void* res;

	switch (allocatorType)
	{
	case (ALLOCATOR_TYPE_MALLOC):
	{
		res = GeneralPurposeAlloc(&GetGameState()->GeneralPurposeMemory, newSize);
	} break;

	case (ALLOCATOR_TYPE_REALLOC):
	{
		res = GeneralPurposeRealloc(&GetGameState()->GeneralPurposeMemory, ptr, newSize);
	} break;

	case (ALLOCATOR_TYPE_FREE):
	{
		GeneralPurposeFree(&GetGameState()->GeneralPurposeMemory, ptr);
		res = nullptr;
	} break;

	default:
		SError("Invalid SAllocator type");
		res = nullptr;
	}

#if SCAL_DEBUG
	if (allocatorType != ALLOCATOR_TYPE_FREE)
		SAssert(res);
#endif

#if TRACK_MEMORY
	HandleMemoryTracking((int)SAllocatorId::General, allocatorType, ptr, oldSize, res, newSize, file, func, line);
#endif

	return res;
}

SAllocatorProc(FrameAllocatorProc)
{
	void* res;

	switch (allocatorType)
	{
	case (ALLOCATOR_TYPE_MALLOC):
	{
		res = ArenaPush(&TransientState.TransientArena, newSize);
	} break;

	case (ALLOCATOR_TYPE_REALLOC):
	{
		res = ArenaPush(&TransientState.TransientArena, newSize);
		if (ptr)
		{
			SCopy(res, ptr, oldSize);
		}
	} break;

	case (ALLOCATOR_TYPE_FREE):
	{
		// Frame SAllocator frees at end of frame
		res = nullptr;
	} break;

	default:
		SError("Invalid SAllocator type");
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

SAllocatorProc(MallocAllocatorProc)
{
	void* res;

	switch (allocatorType)
	{
	case (ALLOCATOR_TYPE_MALLOC):
	{
		res = _aligned_malloc(newSize, align);
	} break;

	case (ALLOCATOR_TYPE_REALLOC):
	{
		res = _aligned_realloc(ptr, newSize, align);
	} break;

	case (ALLOCATOR_TYPE_FREE):
	{
		_aligned_free(ptr);
		res = nullptr;
	} break;

	default:
		SError("Invalid SAllocator type");
		res = nullptr;
	}

#if SCAL_DEBUG
	if (allocatorType != ALLOCATOR_TYPE_FREE)
		SAssert(res);
#endif

#if TRACK_MEMORY
	HandleMemoryTracking((int)SAllocatorId::Malloc, allocatorType, ptr, oldSize, res, newSize, file, func, line);
#endif

	return res;
}

SAllocatorProc(ArenaAllocatorProc)
{
	Arena* arena = (Arena*)data;
	void* res;
	//SInfoLog("Arena alloc %d, %s, %s, %d", newSize, file, func, line);
	switch (allocatorType)
	{
	case (ALLOCATOR_TYPE_MALLOC):
	{
		res = ArenaPush(arena, newSize);
	} break;

	case (ALLOCATOR_TYPE_REALLOC):
	{
		res = ArenaPush(arena, newSize);
		if (ptr)
		{
#if 1
			SAssertMsg(false, "Reallocating an arena allocator. But want to catch any");
#endif
			SMemMove(res, ptr, oldSize);
		}
	} break;

	case (ALLOCATOR_TYPE_FREE):
	{
		SWarn("Do not free ArenaAllocators");
		res = nullptr;
	} break;

	default:
		SError("Invalid SAllocator type");
		res = nullptr;
	}

#if SCAL_DEBUG
	if (allocatorType != ALLOCATOR_TYPE_FREE)
		SAssert(res);
#endif
	return res;
}

