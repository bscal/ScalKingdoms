#pragma once

#define ZPL_HEAP_ANALYSIS
#include <zpl/zpl.h>

#define FLECS_SYSTEM
#include <flecs/flecs.h>

#include <raylib/src/raylib.h>
#undef PI

#include <stdint.h>

#include "Vector2i.h"

/*
*
* -TODO
* -NOTE
* -COMMENT_THIS
* -FIXME
*
*/

static_assert(sizeof(size_t) == sizeof(uint64_t), "ScalEngine does not support 32bit");
static_assert(sizeof(char) == sizeof(uint8_t), "ScalEngine does not support char with sizeof > 1");

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
#define local_static static
#define global static

typedef Vector2i Vec2i;
typedef Vector2 Vec2;
typedef zpl_string String;

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
#define SAPI __declspec(dllexport)
#else
#define SAPI __declspec(dllimport)
#endif // SCAL_BUILD_DLL
#else
#define SAPI
#endif

#define Cast(type) static_cast<type>

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
#define BitSet(state, bit) ((state | 1ULL) << bit)
#define BitClear(state, bit) (state & ~(1ULL << bit))
#define BitToggle(state, bit) (state ^ (1ULL << bit))
#define BitMask(state, mask) (FlagTrue(state, mask))

#define Swap(x, y, T) do { T temp = (x); (x) = (y); (y) = temp; } while(0)

#if SCAL_DEBUG
	#if _MSC_VER
		#define DebugBreak(void) __debugbreak()
	#else
		#define DebugBreak(void) __builtin_trap()
	#endif
	#define SAssert(expr) if (!(expr)) { TraceLog(LOG_ERROR, "Assertion Failure: %s\nMessage: % s\n  File : % s, Line : % d\n", #expr, "", __FILE__, __LINE__); DebugBreak(void); } 
	#define SAssertMsg(expr, msg) if (!(expr)) { TraceLog(LOG_ERROR, "Assertion Failure: %s\nMessage: % s\n  File : % s, Line : % d\n", #expr, msg, __FILE__, __LINE__); DebugBreak(void); }
	#define STraceLog(msg, ...) TraceLog(LOG_TRACE, msg, __VA__ARGS__)
	#define SDebugLog(msg, ...) TraceLog(LOG_DEBUG, msg, __VA_ARGS__)
#else
	#define DebugBreak(void)
	#define SAssert(expr)
	#define SAssertMsg(expr, msg)
	#define STraceLog(msg, ...)
	#define SDebugLog(msg, ...)
#endif

#define SInfoLog(msg, ... ) TraceLog(LOG_INFO, msg, __VA_ARGS__)

#define SWarn(msg, ... ) TraceLog(LOG_WARNING, msg, __VA_ARGS__)

#define SError(msg, ...) \
	TraceLog(LOG_ERROR, msg, __VA_ARGS__); \
	DebugBreak(void) \

#define SFatal(msg, ...) \
	TraceLog(LOG_ERROR, msg, __VA_ARGS__); \
	TraceLog(LOG_FATAL, "Fatal error detected, program crashed! File: %s, Line: %s", __FILE__, __LINE__); \
	DebugBreak(void) \

#define CALL_CONSTRUCTOR(object) new (object)

constexpr global int CACHE_LINE = 64;

constexpr global float PI = (float)3.14159265358979323846;
constexpr global float TAO = PI * 2.0;

constexpr global int WIDTH = 1600;
constexpr global int HEIGHT = 900;

constexpr global const char* TITLE = "Kingdoms";
constexpr global int MAX_FPS = 60;

constexpr global int TILE_SIZE = 16;
constexpr global float INVERSE_TILE_SIZE = 1.0f / TILE_SIZE;
constexpr global float HALF_TILE_SIZE = TILE_SIZE / 2.0f;

constexpr global int CHUNK_SIZE = 32;
constexpr global float INVERSE_CHUNK_SIZE = 1.0f / (float)CHUNK_SIZE;
constexpr global int CHUNK_SIZE_PIXELS = CHUNK_SIZE * TILE_SIZE;
constexpr global int CHUNK_SIZE_PIXELS_HALF = CHUNK_SIZE_PIXELS / 2;
constexpr global int CHUNK_AREA = CHUNK_SIZE * CHUNK_SIZE;

constexpr global int VIEW_RADIUS = 3;
constexpr global int VIEW_DISTANCE_SQR = ((VIEW_RADIUS + 1) * CHUNK_SIZE) * ((VIEW_RADIUS + 1) * CHUNK_SIZE);

enum class Direction : uint8_t
{
	NORTH = 0,
	EAST,
	SOUTH,
	WEST,
	
	MAX_TYPES,
};

constexpr global Vector2 VEC2_ZERO = { 0 , 0 };
constexpr global Vector2 VEC2_UP = { 0 , -1 };
constexpr global Vector2 VEC2_RIGHT = { 1 , 0 };
constexpr global Vector2 VEC2_DOWN = { 0 , 1 };
constexpr global Vector2 VEC2_LEFT = { -1 , 0 };

constexpr global float
TileDirectionToTurns[] = { TAO * 0.75f, 0.0f, TAO * 0.25f, TAO * 0.5f };

#define FMT_VEC2(v) TextFormat("Vector2(x: %.3f, y: %.3f)", v.x, v.y)
#define FMT_VEC2I(v) TextFormat("Vector2i(x: %d, y: %d)", v.x, v.y)
#define FMT_RECT(rect) TextFormat("Rectangle(x: %.3f, y: %.3f, w: %.3f, h: %.3f)", rect.x, rect.y, rect.width, rect.height)
#define FMT_BOOL(boolVar) TextFormat("%s", ((boolVar)) ? "true" : "false")
#define FMT_ENTITY(ent) TextFormat("Entity(%u, Id: %u, Gen: %u", ent, GetId(ent), GetGen(ent))

inline int ScalTestCounter;
inline int ScalTestPasses;

#define ScalTest int ScalTest_

#define ScalTestRun(func) \
	++ScalTestCounter; \
	ScalTestPasses += func()

#define TimerStart(name) Timer(name)
#define TimerStartFunc() TimerStart(__FUNCTION__)

struct Timer
{
	double Start;
	double End;
	const char* Name;

	Timer()
	{
		End = 0;
		Start = zpl_time_rel();
		Name = nullptr;
	}

	Timer(const char* name)
	{
		End = 0;
		Start = zpl_time_rel();
		Name = name;
	}

	~Timer()
	{
		End = zpl_time_rel() - Start;
		if (Name)
			SDebugLog("[ Timer ] Timer %s took %d seconds");
		else
			SDebugLog("[ Timer ] Timer took %d seconds", End);
	}
};
