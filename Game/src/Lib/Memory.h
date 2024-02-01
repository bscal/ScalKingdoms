#pragma once
/*
namespace Memory
{

enum SAllocator
{
    SAllocatorPersistent = 0,
    SAllocatorTransient,
    SAllocatorMalloc,

    SAllocatorMax,
};

struct _NO_VTABLE_ Allocator
{
	virtual void* Alloc(size_t size) = 0;
    virtual void* Calloc(size_t size) = 0;
    virtual void* Realloc(void* ptr, size_t size) = 0;
	virtual void Free(void* ptr) = 0;
};

struct MemoryState
{
	Arena PersistentArena;
	Arena TransientArena;
};

global_var MemoryState GlobalMemoryState;

int MemoryInitialize();

inline bool IsAllocatorValid(SAllocator allocator)
{
    return (int)allocator > 0 && (int)allocator < (int)SAllocatorMax;
}

struct PersistentAllocator final : public Allocator
{
	inline void* Alloc(size_t size) override final
    {
        SAssert(size > 0);
        return ArenaPush(&GlobalMemoryState.PersistentArena, size);
    }

    inline void* Calloc(size_t size) override final
    {
        SAssert(size > 0);
        return ArenaPushZero(&GlobalMemoryState.PersistentArena, size);
    }

    inline void* Realloc(void* ptr, size_t size) override final
    {
        if (!ptr)
        {
            return Alloc(size);
        }
        else
        {
            SError("Calling realloc on arena allocator!");
            return nullptr;
        }
    }

    inline void Free(void* ptr) override final
    {
        SError("Calling free on arena!");
    }
};

struct TransientAllocator final : public Allocator
{
	inline void* Alloc(size_t size) override final
    {
        SAssert(size > 0);
        return ArenaPush(&GlobalMemoryState.TransientArena, size);
    }

    inline void* Calloc(size_t size) override final
    {
        SAssert(size > 0);
        return ArenaPushZero(&GlobalMemoryState.TransientArena, size);
    }

    inline void* Realloc(void* ptr, size_t size) override final
    {
        if (!ptr)
        {
            return Alloc(size);
        }
        else
        {
            SError("Calling realloc on arena allocator!");
            return nullptr;
        }
    }

    inline void Free(void* ptr) override final
    {
        SError("Calling free on arena!");
    }
};


struct MallocAllocator final : public Allocator
{
    inline void* Alloc(size_t size) override final
    {
        SAssert(size > 0);
        return _aligned_malloc(size, DEFAULT_ALIGNMENT);
    }

    inline void* Calloc(size_t size) override final
    {
        SAssert(size > 0);
        void* block = _aligned_recalloc(nullptr, 1, size, DEFAULT_ALIGNMENT);
        return block;
    }

    inline void* Realloc(void* ptr, size_t size) override final
    {
        SAssert(size > 0);
        return _aligned_realloc(ptr, size, DEFAULT_ALIGNMENT);
    }

    inline void Free(void* ptr) override final
    {
        SAssert(ptr);
        _aligned_free(ptr);
    }
};

inline void* 
ProcAlloc(SAllocator allocator, void* ptr, size_t size, size_t oldSize, size_t alignment,
    const char* file, const char* func, int line)
{
    switch (allocator)
    {
        case(SAllocatorPersistent): return PersistentAllocator{}.Alloc(size);
        case(SAllocatorTransient): return TransientAllocator{}.Alloc(size);
        case(SAllocatorMalloc): return MallocAllocator{}.Alloc(size);
        default: SError("Allocator not found");
    }
    return nullptr;
}

inline void* 
ProcCalloc(SAllocator allocator, void* ptr, size_t size, size_t oldSize, size_t alignment,
    const char* file, const char* func, int line)
{
    switch (allocator)
    {
        case(SAllocatorPersistent): return PersistentAllocator{}.Calloc(size);
        case(SAllocatorTransient): return TransientAllocator{}.Calloc(size);
        case(SAllocatorMalloc): return MallocAllocator{}.Calloc(size);
        default: SError("Allocator not found");
    }
    return nullptr;
}

inline void* 
ProcRealloc(SAllocator allocator, void* ptr, size_t size, size_t oldSize, size_t alignment,
    const char* file, const char* func, int line)
{
    switch (allocator)
    {
        case(SAllocatorPersistent): return PersistentAllocator{}.Realloc(ptr, size);
        case(SAllocatorTransient): return TransientAllocator{}.Realloc(ptr, size);
        case(SAllocatorMalloc): return MallocAllocator{}.Realloc(ptr, size);
        default: SError("Allocator not found");
    }
    return nullptr;
}

inline void 
ProcFree(SAllocator allocator, void* ptr, size_t size, size_t oldSize, size_t alignment,
    const char* file, const char* func, int line)
{
    switch (allocator)
    {
        case(SAllocatorPersistent): return PersistentAllocator{}.Free(ptr);
        case(SAllocatorTransient): return TransientAllocator{}.Free(ptr);
        case(SAllocatorMalloc): return MallocAllocator{}.Free(ptr);
        default: SError("Allocator not found");
    }
}

#define SAlloc(allocator, size)	\
	ProcAlloc(allocator, nullptr, size, 0, DEFAULT_ALIGNMENT, __FILE__, __FUNCTION__, __LINE__)

#define SCalloc(allocator, size) \
	ProcCalloc(allocator, nullptr, size, 0, DEFAULT_ALIGNMENT, __FILE__, __FUNCTION__, __LINE__)

#define SRealloc(allocator, ptr, size, oldSize)	\
	ProcRealloc(allocator, ptr, size, oldSize, DEFAULT_ALIGNMENT, __FILE__, __FUNCTION__, __LINE__)

#define SFree(allocator, ptr)   \
	ProcFree(allocator, ptr, 0, 0, DEFAULT_ALIGNMENT, __FILE__, __FUNCTION__, __LINE__)



}
*/