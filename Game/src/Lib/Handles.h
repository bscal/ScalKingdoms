#pragma once

#include "Base.h"
#include "Allocator.h"
#include "Memory.h"

union Handle
{
	u32 Value;
	struct
	{
		u32 Id : 24;
		u32 Gen : 8;
	};
};

struct HandlesPool
{
	SAllocator Allocator;
	u32* Sparse;
	Handle* Dense;
	int Count;
	int Capacity;
};

void HandlesPoolCreate(HandlesPool* handles, SAllocator allocator, int capacity)
{
	SAssert(handles);
	SAssert(IsAllocatorValid(allocator));
	handles->Allocator = allocator;
	handles->Capacity = capacity;
	handles->Sparse = (u32*)SAlloc(handles->Allocator, handles->Capacity * sizeof(u32));
	handles->Dense = (Handle*)SAlloc(handles->Allocator, handles->Capacity * sizeof(Handle));

	for (int i = 0; i < handles->Capacity; ++i)
	{
		handles->Dense[i].Id = i;
	}
}

void HandlesPoolFree(HandlesPool* handles)
{
	SAssert(handles);
	SAssert(handles->Dense);
	SAssert(handles->Sparse);
	SAssert(IsAllocatorValid(handles->Allocator));
	SFree(handles->Allocator, handles->Sparse);
	SFree(handles->Allocator, handles->Dense);
}

void HandlesPoolResize(HandlesPool* handles, int capacity)
{
	SAssert(handles);
	SAssert(IsAllocatorValid(handles->Allocator));

	if (handles->Capacity > capacity)
		return;

	HandlesPool newHandles = {};
	HandlesPoolCreate(&newHandles, handles->Allocator, capacity);

	newHandles.Count = handles->Count;
	SCopy(newHandles.Sparse, handles->Sparse, handles->Capacity * sizeof(u32));
	SCopy(newHandles.Dense, handles->Dense, handles->Capacity * sizeof(Handle));

	HandlesPoolFree(handles);
	*handles = newHandles;
}

Handle HandlesPoolNew(HandlesPool* handles)
{
	SAssert(handles);
	SAssert(handles->Dense);
	SAssert(handles->Sparse);

	if (handles->Count < handles->Capacity)
	{
		int idx = handles->Count;
		++handles->Count;

		handles->Dense[idx].Gen++;
		handles->Sparse[handles->Dense[idx].Id] = idx;

		return handles->Dense[idx];
	}
	else
	{
		SError("HandlesPool has no more free ids");
		return {};
	}
}

void HandlesPoolDel(HandlesPool* handles, Handle handle)
{
	SAssert(handles);
	SAssert(handles->Dense);
	SAssert(handles->Sparse);
	SAssert(handles->Count > 0);
	SAssert(HandlesPoolValid(handles, handle));

	int idx = handles->Sparse[handle.Id];

	Handle lastHandle = handles->Dense[handles->Count - 1];

	handles->Sparse[lastHandle.Id] = idx;
	handles->Dense[idx] = lastHandle;
	handles->Dense[handles->Count - 1] = handle;
}

bool HandlesPoolValid(HandlesPool* handles, Handle handle)
{
	SAssert(handles);
	SAssert(handles->Dense);
	SAssert(handles->Sparse);
	SAssert(handles->Count > 0);

	return handle.Id < handles->Count && handles->Dense[handle.Id].Value == handle.Value;
}