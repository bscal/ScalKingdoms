#pragma once

#include "Core.h"

#include "Structures/HashMap.h"
#include "Structures/HashSet.h"
#include "Structures/BHeap.h"

struct TileMap;
struct Chunk;
struct CMove;

constexpr global_var int DIVISIONS = 4;
constexpr global_var int REGION_SIZE = CHUNK_SIZE / DIVISIONS;

struct RegionPaths
{
	Vec2i Pos;
	bool Sides[4];
};

struct RegionState
{
	HashMap RegionMap;
	BHeap* Open;
	HashMap OpenMap;
	HashSet ClosedSet;
};

void RegionStateInit(RegionState* regionState);

void LoadRegionPaths(RegionState* regionState, TileMap* tilemap, Chunk* chunk);

void UnloadRegionPaths(RegionState* regionState, Chunk* chunk);

void FindRegionPath(RegionState* regionState, TileMap* tilemap, Vec2i start, Vec2i end, HashSet* regionSet);

void RegionFindLocalPath(RegionState* regionState, Vec2i start, Vec2i end, CMove* moveComponent);

RegionPaths* GetRegion(RegionState* regionState, Vec2i tilePos);

