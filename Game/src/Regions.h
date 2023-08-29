#pragma once

#include "Core.h"

#include "Structures/ArrayList.h"
#include "Structures/HashMap.h"
#include "Structures/HashSet.h"
#include "Structures/BHeap.h"

#include <klib/kbtree.h>

struct TileMap;
struct Chunk;

constexpr global_var int REGION_SIZE = 16;

struct Region
{
	zpl_array(ecs_entity_t) Entities;
	Vec2i Start;
	Vec2i End;
	Region* Neighbors[4];
};

struct RegionPaths
{
	Vec2i Nodes[4];
	Vec2i NeightborChunks[4];
	int ChunkCost;
};

struct RegionNode
{
	Vec2i Pos;
	RegionNode* Parent;
	int MoveDirection;
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

void LoadRegionPaths(RegionState* regionState, RegionPaths* regionPath, TileMap* tilemap, Chunk* chunk);

void UnloadRegionPaths();

void FindRegionPath(RegionState* regionState, TileMap* tilemap, Vec2i start, Vec2i end, zpl_array(int) arr);

Vec2i RegionGetNode(RegionState* regionState, RegionPaths* region, int direction);

void RegionFindLocalPath(RegionState* regionState, Vec2i start, Vec2i end, zpl_array(Vec2i) arr);

RegionPaths* GetRegion(RegionState* regionState, Vec2i tilePos);

