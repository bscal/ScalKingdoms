#pragma once

#include <stdint.h>

static_assert(sizeof(size_t) == sizeof(unsigned long long), "ScalEngine does not support 32bit");
static_assert(sizeof(int) == sizeof(long), "sizeof int != sizeof long");
static_assert(sizeof(int) == sizeof(float), "sizeof int != sizeof float");
static_assert(sizeof(char) == 1, "sizeof char does not == 1");
static_assert(sizeof(uint8_t) == 1, "sizeof uint8_t does not == 1");

typedef int bool32;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#define internal static
#define internal_var static
#define global_var inline
#define constant_var constexpr static
#define local_persist static

#define PI 3.14159265358979323846f
constant_var float TAO = PI * 2.0f;
constant_var size_t CACHE_LINE = 64;
constant_var size_t DEFAULT_ALIGNMENT = 16;

#ifdef SCAL_DEBUG
#ifdef _MSC_VER
#define DebugBreak(void) __debugbreak()
#else
#define DebugBreak(void) __builtin_trap()
#endif
#else
#define DebugBreak(void)
#endif

#define C_DECL_BEGIN extern "C" {
#define C_DECL_END }

#if defined(__clang__) || defined(__GNUC__)
#define _RESTRICT_ __restrict__
#elif defined(_MSC_VER)
#define _RESTRICT_ __restrict
#else
#define _RESTRICT_
#endif

#ifndef _ALWAYS_INLINE_
#if defined(__clang__) || defined(__GNUC__)
#define _ALWAYS_INLINE_ __attribute__((always_inline)) inline
#elif defined(_MSC_VER)
#define _ALWAYS_INLINE_ __forceinline
#else
#define _ALWAYS_INLINE_ inline
#endif
#endif

#ifndef _FORCE_INLINE_
#ifdef SCAL_DEBUG
#define _FORCE_INLINE_ inline
#else
#define _FORCE_INLINE_ _ALWAYS_INLINE_
#endif
#endif

#ifndef _NEVER_INLINE
#if defined(__clang__) || defined(__GNUC__)
#define _NEVER_INLINE __attribute__((__noinline__)) inline
#elif defined(_MSC_VER)
#define _NEVER_INLINE __declspec(noinline)
#else
#define _NEVER_INLINE 
#endif
#endif

#ifdef SCAL_PLATFORM_WINDOWS
#ifdef SCAL_BUILD_DLL
#define SAPI extern "C" __declspec(dllexport)
#else
#define SAPI extern "C" __declspec(dllimport)
#endif // SCAL_BUILD_DLL
#else
#define SAPI extern "C"
#endif

#define Unpack(...) __VA_ARGS__
#define Stringify(x) #x
#define Expand(x) Stringify(x)

#define ArrayLength(arr) (sizeof(arr) / sizeof(arr[0]))

#define Kilobytes(n) (n * 1024ULL)
#define Megabytes(n) (Kilobytes(n) * 1024ULL)
#define Gigabytes(n) (Megabytes(n) * 1024ULL)

#define Bit(x) (1ULL<<(x))

#define FlagTrue(state, flag) ((state & flag) == flag)
#define FlagFalse(state, flag) ((state & flag) != flag)

#define BitGet(state, bit) ((state >> bit) & 1ULL)
#define BitSet(state, bit) (state | (1ULL << bit))
#define BitClear(state, bit) (state & ~(1ULL << bit))
#define BitToggle(state, bit) (state ^ (1ULL << bit))
#define BitMask(state, mask) (FlagTrue(state, mask))

#define Swap(x, y, T) do { T temp = (x); (x) = (y); (y) = temp; } while(0)

#define Min(v0, v1) ((v0 < v1) ? v0 : v1)
#define Max(v0, v1) ((v0 > v1) ? v0 : v1)
#define ClampValue(v, min, max) Min(max, Max(min, v))

#define CallConstructor(object, T) new (object) T

// Double linked list : First, Last, Node
#define DLPushBackT(f, l, n, Next, Prev) (((f) == 0 \
											? (f) = (l) = (n), (n)->Next = (n)->Prev = 0) \
											: ((n)->Prev = (l), (l)->Next = (n), (l) = (n), (n)->Next = 0))
#define DLPushBack(f, l, n) DLPushBackT(f, l, n, Next, Prev)

#define DLPushFront(f, l, n) DLPushBackT(f, l, n, Prev, Next)

#define DLRemoveT(f, l, n, Next, Prev) (((f) == (n) \
											? ((f) = (f)->Next, (f)->Prev = 0) \
											: (l) == (n) \
												? ((l) = (l)->Prev, (l)->Next = 0) \
												: ((n)->Next->Prev = (n)->Prev \
													, (n)->Prev->Next = (n)->Next)))

#define DLRemove(f, l, n) ) DLRemoveT(f, l, n, Next, Prev)

// Singled linked list queue : First, Last, Node
#define SLQueuePush(f, l, n, Next) ((f) == 0 \
									? (f) = (l) = (n) \
									: ((l)->Next = (n), (l) = (n)) \
										, (n)->Next = 0)

#define SLQueuePushFront(f, l, n, Next) ((f) == 0 \
											? (f) = (l) = (n), (n)->Next = 0) \
											: ((n)->Next = (f), (f) = (n))) \

#define SLQueuePop(f, l) ((f) == (l) \
								? (f) = (l) = 0 \
								: (f) = (f)->Next)

// Singled linked list stack : First, Node
#define SLStackPush(f, n, Next) ((n)->Next = (f), (f) = (n))

#define SLStackPop(f, Next) ((f) == 0 ? 0 : (f) = (f)->Next)