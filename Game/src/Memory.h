#pragma once

#include "Core.h"

#define SPersistent zpl_heap_allocator()

#define SAlloc(allocator, size) zpl_alloc(allocator, size)
#define SFree(allocator, ptr) zpl_free(allocator, ptr)

#define SMemZero(ptr, size) zpl_memset(ptr, 0, size)