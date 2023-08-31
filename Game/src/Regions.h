#pragma once

#include "Core.h"

#include "Structures/ArrayList.h"
#include "Structures/HashMap.h"
#include "Structures/HashSet.h"
#include "Structures/BHeap.h"

#include <klib/kbtree.h>

struct TileMap;
struct Chunk;
struct CMove;

constexpr global_var int DIVISIONS = 4;
constexpr global_var int REGION_SIZE = CHUNK_SIZE / DIVISIONS;

struct Region
{
	zpl_array(ecs_entity_t) Entities;
	Vec2i Start;
	Vec2i End;
	Region* Neighbors[4];
};

struct RegionPaths
{
	Vec2i Pos;
	bool Sides[4];
};

struct RegionNode
{
	Vec2i Pos;
	RegionNode* Parent;
	int FCost;
	int HCost;
	int GCost;
};


//#define RegionCompare(a, b) (CompareCost(a, b))

//KBTREE_INIT(regions, RegionNode, RegionCompare);

struct RegionState
{
	HashMap RegionMap;
	BHeap* Open;
	HashMap OpenMap;
	//kbtree_t(regions) OpenSet;
	HashSet ClosedSet;
};

void RegionStateInit(RegionState* regionState);

void LoadRegionPaths(RegionState* regionState, TileMap* tilemap, Chunk* chunk);

void UnloadRegionPaths();

void FindRegionPath(RegionState* regionState, TileMap* tilemap, Vec2i start, Vec2i end, HashSet* regionSet);

void RegionFindLocalPath(RegionState* regionState, Vec2i start, Vec2i end, CMove* moveComponent);

RegionPaths* GetRegion(RegionState* regionState, Vec2i tilePos);

