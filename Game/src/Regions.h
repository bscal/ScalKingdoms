#pragma once

#include "Core.h"

#include "Structures/HashMap.h"
#include "Structures/HashSet.h"
#include "Structures/BHeap.h"
#include "Structures/ArrayList.h"
#include "Structures/HashMapT.h"
#include "Structures/HashSetT.h"
#include "Structures/HashMapKV.h"

struct TileMap;
struct Chunk;
struct CMove;

constexpr global_var int DIVISIONS = 4;
constexpr global_var int REGION_SIZE = CHUNK_SIZE / DIVISIONS;

struct RegionPaths
{
	ArrayList(ecs_entity_t) Creatures;
	Vec2i Pos;
	Vec2i TilePos;
	bool Sides[4];
};

struct RegionState
{
	HashMapKV RegionMap;
	BHeap* Open;
	HashMapT<Vec2i, int> OpenMap;
	HashSetT<Vec2i> ClosedSet;
};

void RegionStateInit(RegionState* regionState);

void LoadRegionPaths(RegionState* regionState, TileMap* tilemap, Chunk* chunk);

void UnloadRegionPaths(RegionState* regionState, Chunk* chunk);

void FindRegionPath(RegionState* regionState, TileMap* tilemap, Vec2i start, Vec2i end, HashSetT<Vec2i>* regionSet);

void RegionFindLocalPath(RegionState* regionState, Vec2i start, Vec2i end, CMove* moveComponent);

RegionPaths* GetRegion(RegionState* regionState, Vec2i tilePos);

