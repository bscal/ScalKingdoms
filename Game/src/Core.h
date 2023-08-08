#pragma once

#include <stdint.h>
#include <zpl/zpl.h>
#include <raylib/src/raylib.h>

#define FLECS_SYSTEM
#include <flecs/flecs.h>

#include "Memory.h"
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
typedef Vector2i Vec2i;
typedef Vector2 Vec2;

#define internal static
#define local_var static
#define global_var static

#if defined(__clang__) || defined(__GNUC__)
#define _RESTRICT_ __restrict__
#elif defined(_MSC_VER)
#define _RESTRICT_ __restrict
#else
#define _RESTRICT_
#endif

#ifndef _ALWAYS_INLINE_
#if defined(__GNUC__)
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

#ifdef SCAL_PLATFORM_WINDOWS
#ifdef SCAL_BUILD_DLL
#define SAPI __declspec(dllexport)
#else
#define SAPI __declspec(dllimport)
#endif // SCAL_BUILD_DLL
#else
#define SAPI
#endif

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

#define Swap(x, y, T) (T temp = x; x = y; y = temp)

#if SCAL_DEBUG

	#if _MSC_VER
		#define DEBUG_BREAK(void) __debugbreak()
	#else
		#define DEBUG_BREAK(void) __builtin_trap()
	#endif

	#define SASSERT(expr) if (!(expr)) { TraceLog(LOG_ERROR, "Assertion Failure: %s\nMessage: % s\n  File : % s, Line : % d\n", #expr, "", __FILE__, __LINE__); DEBUG_BREAK(void); } 
	#define SASSERT_MSG(expr, msg) if (!(expr)) { TraceLog(LOG_ERROR, "Assertion Failure: %s\nMessage: % s\n  File : % s, Line : % d\n", #expr, msg, __FILE__, __LINE__); DEBUG_BREAK(void); }


#else
	#define DEBUG_BREAK(void)
	#define SASSERT(expr)
	#define SASSERT_MSG(expr, msg)
#endif

#define SLOG_INFO(msg, ... ) TraceLog(LOG_INFO, msg, __VA_ARGS__)
#define SLOG_DEBUG(msg, ...) TraceLog(LOG_DEBUG, msg, __VA_ARGS__)

#define SWARN(msg, ... ) TraceLog(LOG_WARNING, msg, __VA_ARGS__)

#define SERR(msg, ...) \
	TraceLog(LOG_ERROR, msg, __VA_ARGS__); \
	DEBUG_BREAK(void) \

#define SFATAL(msg, ...) \
	TraceLog(LOG_ERROR, msg, __VA_ARGS__); \
	TraceLog(LOG_FATAL, "Fatal error detected, program crashed! File: %s, Line: %s", __FILE__, __LINE__); \
	DEBUG_BREAK(void) \

#define CALL_CONSTRUCTOR(object) new (object)

constexpr global_var float TAO = PI * 2.0;

constexpr global_var int WIDTH = 1600;
constexpr global_var int HEIGHT = 900;

constexpr global_var const char* TITLE = "Kingdoms";
constexpr global_var int MAX_FPS = 60;

constexpr global_var int TILE_SIZE = 16;
constexpr global_var float INVERSE_TILE_SIZE = 1.0f / TILE_SIZE;
constexpr global_var float HALF_TILE_SIZE = TILE_SIZE / 2.0f;

constexpr global_var int CHUNK_SIZE = 32;
constexpr global_var float INVERSE_CHUNK_SIZE = 1.0f / (float)CHUNK_SIZE;
constexpr global_var int CHUNK_SIZE_PIXELS = CHUNK_SIZE * TILE_SIZE;
constexpr global_var int CHUNK_SIZE_PIXELS_HALF = CHUNK_SIZE_PIXELS / 2;
constexpr global_var int CHUNK_AREA = CHUNK_SIZE * CHUNK_SIZE;

constexpr global_var int VIEW_RADIUS = 2;
constexpr global_var int VIEW_DISTANCE_TOTAL_CHUNKS = (VIEW_RADIUS * 2 + 1) * (VIEW_RADIUS * 2 + 1) + 1;
constexpr global_var int VIEW_DISTANCE_SQR = ((VIEW_RADIUS + 1) * CHUNK_SIZE) * ((VIEW_RADIUS + 1) * CHUNK_SIZE);

enum class Direction : uint8_t
{
	NORTH,
	EAST,
	SOUTH,
	WEST
};

constexpr global_var Vector2 VEC2_ZERO = { 0 , 0 };
constexpr global_var Vector2 VEC2_UP = { 0 , -1 };
constexpr global_var Vector2 VEC2_RIGHT = { 1 , 0 };
constexpr global_var Vector2 VEC2_DOWN = { 0 , 1 };
constexpr global_var Vector2 VEC2_LEFT = { -1 , 0 };

constexpr global_var float
TileDirectionToTurns[] = { TAO * 0.75f, 0.0f, TAO * 0.25f, TAO * 0.5f };

constexpr global_var Vector2
TileDirectionVectors[] = {{0, -TILE_SIZE}, {TILE_SIZE, 0}, {0, TILE_SIZE}, {-TILE_SIZE, 0}};

constexpr global_var Vector2i
TileDirectionVectorInts[] = { {0, -1}, {1, 0}, {0, 1}, {-1, 0} };

#define FMT_VEC2(v) TextFormat("Vector2(x: %.3f, y: %.3f)", v.x, v.y)
#define FMT_VEC2I(v) TextFormat("Vector2i(x: %d, y: %d)", v.x, v.y)
#define FMT_RECT(rect) TextFormat("Rectangle(x: %.3f, y: %.3f, w: %.3f, h: %.3f)", rect.x, rect.y, rect.width, rect.height)
#define FMT_BOOL(boolVar) TextFormat("%s", ((boolVar)) ? "true" : "false")
#define FMT_ENTITY(ent) TextFormat("Entity(%u, Id: %u, Gen: %u", ent, GetId(ent), GetGen(ent))

#define HashMapAllocate(size, oldSize) zpl_alloc(zpl_heap_allocator(), size);
#define HashMapFree(ptr, oldSize) zpl_free(zpl_heap_allocator(), ptr);

constexpr int IntModNegative(int a, int b)
{
	int res = a % b;
	return (res < 0) ? res + b : res;
}

_FORCE_INLINE_ double GetMicroTime()
{
	return GetTime() * 1000000.0;
}

_FORCE_INLINE_ constexpr size_t
AlignPowTwo64(size_t num)
{
	if (num == 0) return 0;

	size_t power = 1;
	while (num >>= 1) power <<= 1;
	return power;
}

_FORCE_INLINE_ constexpr uint32_t
AlignPowTwo32(uint32_t num)
{
	if (num == 0) return 0;

	uint32_t power = 1;
	while (num >>= 1) power <<= 1;
	return power;
}

_FORCE_INLINE_ constexpr size_t
AlignPowTwo64Ceil(size_t x)
{
	if (x <= 1) return 1;
	size_t power = 2;
	--x;
	while (x >>= 1) power <<= 1;
	return power;
}

_FORCE_INLINE_ constexpr uint32_t
AlignPowTwo32Ceil(uint32_t x)
{
	if (x <= 1) return 1;
	uint32_t power = 2;
	--x;
	while (x >>= 1) power <<= 1;
	return power;
}

_FORCE_INLINE_ constexpr bool
IsPowerOf2_32(uint32_t num)
{
	return (num > 0 && ((num & (num - 1)) == 0));
}

_FORCE_INLINE_ constexpr size_t
AlignSize(size_t size, size_t alignment)
{
	return (size + (alignment - 1)) & ~(alignment - 1);
}

// TODO move
inline void 
TextSplitBuffered(
	const char* _RESTRICT_ text,
	char delimiter,
	int* _RESTRICT_ count,
	char* _RESTRICT_ buffer,
	int bufferLength,
	char** _RESTRICT_ splitBuffer,
	int splitBufferLength)
{
	splitBuffer[0] = buffer;
	int counter = 0;

	if (text != NULL)
	{
		counter = 1;

		// Count how many substrings we have on text and point to every one
		for (int i = 0; i < bufferLength; i++)
		{
			buffer[i] = text[i];
			if (buffer[i] == '\0') break;
			else if (buffer[i] == delimiter)
			{
				buffer[i] = '\0';   // Set an end of string at this point
				splitBuffer[counter] = buffer + i + 1;
				counter++;

				if (counter == splitBufferLength) break;
			}
		}
	}

	*count = counter;
}

constexpr void 
RemoveWhitespace(char* s)
{
	char* d = s;
	do {
		while (*d == ' ') {
			++d;
		}
	} while (*s++ = *d++);
}

enum STR2INT
{
	STR2INT_SUCCESS,
	STR2INT_OVERFLOW,
	STR2INT_UNDERFLOW,
	STR2INT_INCONVERTIBLE
};

inline STR2INT Str2Int(int* out, const char* s, int base)
{
	char* end;
	if (s[0] == '\0' || isspace(s[0]))
		return STR2INT_INCONVERTIBLE;
	errno = 0;
	long l = strtol(s, &end, base);
	/* Both checks are needed because INT_MAX == LONG_MAX is possible. */
	if (l > INT_MAX || (errno == ERANGE && l == LONG_MAX))
		return STR2INT_OVERFLOW;
	if (l < INT_MIN || (errno == ERANGE && l == LONG_MIN))
		return STR2INT_UNDERFLOW;
	if (*end != '\0')
		return STR2INT_INCONVERTIBLE;
	*out = l;
	return STR2INT_SUCCESS;
}
