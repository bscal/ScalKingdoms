#pragma once

// ZPL impl does not play well with raylib because windows.h
#include <zpl/zpl.h>

#define FLECS_SYSTEM
#include <flecs/flecs.h>

#include <raylib/src/raylib.h>
#include <raylib/src/raymath.h>

#include "Base.h"
#include "Vector2i.h"
#include "RectI.h"
#include "Utils.h"
#include "Debug.h"
#include "Memory.h"
#include "Lib/Random.h"

/*
*
* -TODO
* -NOTE
* -COMMENT_THIS
* -FIXME
*
*/

typedef Vector2i Vec2i;
typedef Vector2 Vec2;

#if SCAL_DEBUG
	#define SAssert(expr) if (!(expr)) { TraceLog(LOG_ERROR, "Assertion Failure: %s\nMessage: % s\n  File : % s, Line : % d\n", #expr, "", __FILE__, __LINE__); DebugBreak(void); } 
	#define SAssertMsg(expr, msg) if (!(expr)) { TraceLog(LOG_ERROR, "Assertion Failure: %s\nMessage: % s\n  File : % s, Line : % d\n", #expr, msg, __FILE__, __LINE__); DebugBreak(void); }
	#define STraceLog(msg, ...) TraceLog(LOG_TRACE, msg, __VA__ARGS__)
	#define SDebugLog(msg, ...) TraceLog(LOG_DEBUG, msg, __VA_ARGS__)
#else
	#define SAssert(expr)
	#define SAssertMsg(expr, msg)
	#define STraceLog(msg, ...)
	#define SDebugLog(msg, ...)
#endif

#define SInfoLog(msg, ... ) TraceLog(LOG_INFO, msg, __VA_ARGS__)

#define SWarnAlwaysAssert 0
#define SWarn(msg, ... ) \
	TriggerErrorPopupWindow(true, TextFormat(msg, __VA_ARGS__), __FILE__, __FUNCTION__, __LINE__) \

#define SError(msg, ...) \
	TraceLog(LOG_ERROR, msg, __VA_ARGS__); \
	DebugBreak(void) \

#define SFatal(msg, ...) \
	TraceLog(LOG_ERROR, msg, __VA_ARGS__); \
	TraceLog(LOG_FATAL, "Fatal error detected, program crashed! File: %s, Line: %s", __FILE__, __LINE__); \
	DebugBreak(void) \

constant_var int WIDTH = 1920;
constant_var int HEIGHT = 1080;
constant_var int GAME_WIDTH = 1600;
constant_var int GAME_HEIGHT = 900;

constant_var const char* TITLE = "Kingdoms";
constant_var int MAX_FPS = 60;

constant_var int TILE_SIZE = 16;
constant_var float INVERSE_TILE_SIZE = 1.0f / TILE_SIZE;
constant_var float HALF_TILE_SIZE = TILE_SIZE / 2.0f;

constant_var int CHUNK_SIZE = 32;
constant_var float INVERSE_CHUNK_SIZE = 1.0f / (float)CHUNK_SIZE;
constant_var int CHUNK_SIZE_PIXELS = CHUNK_SIZE * TILE_SIZE;
constant_var int CHUNK_SIZE_PIXELS_HALF = CHUNK_SIZE_PIXELS / 2;
constant_var int CHUNK_AREA = CHUNK_SIZE * CHUNK_SIZE;

constant_var int VIEW_RADIUS = 3;
constant_var int VIEW_DISTANCE_SQR = ((VIEW_RADIUS + 1) * CHUNK_SIZE) * ((VIEW_RADIUS + 1) * CHUNK_SIZE);
constant_var int VIEW_DISTANCE_SIZE = VIEW_RADIUS * 2 + 1;
constant_var int VIEW_DISTANCE_TOTAL_CHUNKS = (VIEW_DISTANCE_SIZE + 1) * (VIEW_DISTANCE_SIZE + 1);

constant_var int VIEW_WIDTH = 4;
constant_var int VIEW_WIDTH_TILES = VIEW_WIDTH * CHUNK_SIZE;
constant_var int VIEW_SIZE_TILES = VIEW_WIDTH_TILES * VIEW_WIDTH_TILES;
constant_var int VIEW_MIDDLE_TILES = VIEW_WIDTH_TILES / 2;

enum class Direction : uint8_t
{
	NORTH = 0,
	EAST,
	SOUTH,
	WEST,
	
	MAX_TYPES,
};

constant_var Vector2 VEC2_ZERO = { 0 , 0 };
constant_var Vector2 VEC2_UP = { 0 , -1 };
constant_var Vector2 VEC2_RIGHT = { 1 , 0 };
constant_var Vector2 VEC2_DOWN = { 0 , 1 };
constant_var Vector2 VEC2_LEFT = { -1 , 0 };

constant_var float
TileDirectionToTurns[] = { TAO * 0.75f, 0.0f, TAO * 0.25f, TAO * 0.5f };

//
// Vector2 operator overloads
//
static inline Vector2 operator+(Vector2 left, Vector2 right)
{
	return Vector2Add(left, right);
}

static inline Vector2 operator-(Vector2 left, Vector2 right)
{
	return Vector2Subtract(left, right);
}

static inline Vector2 operator*(Vector2 left, Vector2 right)
{
	return Vector2Multiply(left, right);
}

static inline Vector2 operator/(Vector2 left, Vector2 right)
{
	return Vector2Divide(left, right);
}

#define FMT_VEC2(v) TextFormat("Vector2(x: %.3f, y: %.3f)", v.x, v.y)
#define FMT_VEC2I(v) TextFormat("Vector2i(x: %d, y: %d)", v.x, v.y)
#define FMT_RECT(rect) TextFormat("Rectangle(x: %.3f, y: %.3f, w: %.3f, h: %.3f)", rect.x, rect.y, rect.width, rect.height)
#define FMT_BOOL(boolVar) TextFormat("%s", ((boolVar)) ? "true" : "false")
#define FMT_ENTITY(ent) TextFormat("Entity(%u, Id: %u, Gen: %u", ent, GetId(ent), GetGen(ent))
