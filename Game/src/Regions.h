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
	Region* CurRegion;
	ArrayList(PortalConnection) PortalConnections;
};

struct Region
{
	ArrayList(Portal) Portals;
};

void RegionInit2(TileMap* tilemap, Chunk* chunk);
void PathfindNodes(Pathfinder* pathfinder, Vec2i start, Vec2i end, void(*callback)(Node*, void*), void* stack);
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

