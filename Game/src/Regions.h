#pragma once

#include "Core.h"

#include "Structures/StaticArray.h"
#include "Structures/BHeap.h"
#include "Structures/HashMapT.h"
#include "Structures/HashSetT.h"

struct TileMap;
struct Chunk;
struct CMove;

constant_var int DIVISIONS = 4;
constant_var int REGION_SIZE = CHUNK_SIZE / DIVISIONS;

// Order of tiles we check on side to get one most near middle of a region side
constant_var u8 REGION_SIDE_POINTS[REGION_SIZE] = { 3, 4, 2, 5, 1, 6, 0, 7 };

enum class RegionDirection : int
{
	N2W,
	N2S,
	N2E,
	E2N,
	E2W,
	E2S,
	S2E,
	S2N,
	S2W,
	W2S,
	W2E,
	W2N,

	MAX
};
constant_var int REGION_DIR_MAX = (int)RegionDirection::MAX;
constant_var u8 REGION_DIR_START[REGION_DIR_MAX] = { 0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 3 };
constant_var u8 REGION_DIR_END[REGION_DIR_MAX] = { 3, 2, 1, 0, 3, 2, 1, 0, 3, 2, 1, 0 };
constant_var u8 DIRECTION_2_REGION_DIR[4][4] =
{
	{ 0, 2, 1, 0 },
	{ 3, 0, 5, 4 },
	{ 7, 6, 0, 8 },
	{ 11, 10, 9, 0 }
};

struct RegionNode
{
	Vec2i Pos;
	RegionNode* Parent;
	int GCost;
	int HCost;
	int FCost;
	u8 SideFrom;
};

struct RegionPathfinder
{
	BHeap* Open;
	HashMapT<Vec2i, RegionNode*> OpenSet;
	HashSetT<Vec2i> ClosedSet;
};

struct Region
{
	Vec2i Coord;
	Vec2i Sides[4];
	Vec2i SideConnections[4];
	short PathCost[REGION_DIR_MAX];
	u8 PathLengths[REGION_DIR_MAX];
	Vec2i PathPaths[REGION_DIR_MAX][32];
};

struct RegionPath
{
	Vec2i RegionCoord;
	RegionDirection Direction;
};

struct RegionMoveData
{
	Vec2i StartRegionTilePos;
	Vec2i EndRegionTilePos;
	int PathProgress;
	Buffer<RegionPath, 128> Path;	// Regions path
	Buffer<Vec2i, 64> StartPath;	// Path from start -> first region in Path, or to end position if no regions
	Buffer<Vec2i, 32> EndPath;		// Last region to end position
};

void PathfinderRegionsInit(RegionPathfinder* pathfinder);

void RegionLoad(TileMap* tilemap, Chunk* chunk);
void RegionUnload(Chunk* chunk);

void PathfindRegion(Vec2i tileStart, Vec2i tileEnd, RegionMoveData* moveData);

void DrawRegions();

Region* GetRegion(Vec2i tilePos);
