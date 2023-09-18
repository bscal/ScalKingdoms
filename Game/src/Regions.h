#pragma once

#include "Core.h"

#include "Structures/HashMap.h"
#include "Structures/HashSet.h"
#include "Structures/BHeap.h"
#include "Structures/ArrayList.h"
#include "Structures/HashMapT.h"
#include "Structures/HashSetT.h"

struct TileMap;
struct Chunk;
struct CMove;
struct Portal;
struct Node;
struct Pathfinder;

constexpr global_var int DIVISIONS = 4;
constexpr global_var int REGION_SIZE = CHUNK_SIZE / DIVISIONS;

constexpr global_var u8 REGION_SIDE_POINTS[REGION_SIZE] = { 3, 4, 2, 5, 1, 6, 0, 7 };

struct Region;

struct FlowTile
{
	u8 Dir : 4;
	u8 Pathable : 1;
	u8 LOS : 1;
};
static_assert(sizeof(FlowTile) == 1);

enum class RegionDirection : u8
{
	N2W,
	N2S,
	N2E,
	E2N,
	E2W,
	E2S,
	S2E,//
	S2N,
	S2W,
	W2S,
	W2E,
	W2N,

	MAX
};
constexpr int REGION_DIR_MAX = (int)RegionDirection::MAX;
constexpr u8 REGION_DIR_START[REGION_DIR_MAX] = { 0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 3 };
constexpr u8 REGION_DIR_END[REGION_DIR_MAX] = { 3, 2, 1, 0, 3, 2, 1, 0, 3, 2, 1, 0 };
constexpr u8 DIRECTION_2_REGION_DIR[4][4] =
{
	{ 0, 2, 1, 0 },
	{ 3, 0, 5, 4 },
	{ 7, 6, 0, 8 },
	{ 11, 10, 9, 0 }
};

struct RegionPath
{
	Vec2i RegionCoord;
	RegionDirection Direction;
};

struct RegionNode
{
	Vec2i Pos;
	RegionNode* Parent;
	int GCost;
	int HCost;
	int FCost;
	u8 SideFrom;
	u8 SideTo;
};

struct Region
{
	Vec2i Coord;
	Vec2i Sides[4];
	Vec2i SideConnections[4];
	int PathCost[REGION_DIR_MAX];
	ArrayList(Vec2i) Paths[REGION_DIR_MAX];
};

struct RegionMoveData
{
	Vec2i StartRegionTilePos;
	ArrayList(Vec2i) StartPath;
	Vec2i EndRegionTilePos;
	ArrayList(Vec2i) EndPath;
	ArrayList(RegionPath) RegionPath;
	bool NeedToPathfindToStart;
	bool NeedToPathfindToEnd;
};

void RegionInit2(TileMap* tilemap, Chunk* chunk);
void PathfindRegion(Vec2i tileStart, Vec2i tileEnd, RegionMoveData* moveData);
void DrawRegions();

struct RegionPaths
{
	Vec2i Pos;
	Vec2i TilePos;
	bool Sides[4];
	Vec2i Portals[8];
};

struct RegionState
{
	HashMap RegionMap;
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

