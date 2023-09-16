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



struct PortalConnection
{
	Vec2i OtherPortal;
	int Cost;
	ArrayList(Vec2i) Path;
};

struct Portal
{
	Vec2i Pos;
	Vec2i ConnectedPortalPos;
	Vec2i CurRegion;
	ArrayList(PortalConnection) PortalConnections;
};

enum class RegionDirection : u8
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
constexpr int REGION_DIR_MAX = (int)RegionDirection::MAX;
constexpr u8 REGION_DIR_START[REGION_DIR_MAX] = { 0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 3 };
constexpr u8 REGION_DIR_END[REGION_DIR_MAX] = { 3, 2, 1, 0, 3, 2, 1, 0, 3, 2, 1, 0 };

struct RegionPath
{
	Region* Current;
	Region* From;
	int DirectionCur;
	int DirectionFrom;
	RegionDirection Direction;
};

struct Region
{
	ArrayList(Portal) Portals;

	Vec2i Coord;
	Vec2i Sides[4];
	int PathCost[REGION_DIR_MAX];
	ArrayList(Vec2i) Paths[REGION_DIR_MAX];
};

struct PortalSearchData
{
	Portal* Portal;
	PortalSearchData* Parent;

	friend inline bool operator==(PortalSearchData left, PortalSearchData right)
	{
		return left.Portal == right.Portal && left.Parent == right.Parent;
	}
};

void RegionInit2(TileMap* tilemap, Chunk* chunk);
void PathfindNodes(Vec2i start, Vec2i end, void(*callback)(PortalSearchData*, void*), void* stack);
//void PortalGenerate(Portal* portal, TileMap* tilemap, Chunk* chunk, const ArrayList(u8) costArray);
Portal* PortalFind(Vec2i pos);
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

