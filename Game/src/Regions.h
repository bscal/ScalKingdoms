#pragma once

#include "Core.h"

#include "Structures/ArrayList.h"
#include "Structures/HashMap.h"

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
	Vec2i ChunkStart;
	Vec2i ChunkEnd;
	ArrayList(Vec2i) Tiles;
};

void LoadRegionPaths(RegionPaths* regionPath, TileMap* tilemap, Chunk* chunk);

void UnloadRegionPaths();

zpl_array(Vec2i) FindRegionPath();

//

void LoadRegion(Region* region, TileMap* tilemap, Chunk* chunk);

void UnloadRegion(Region* region);

Region* GetRegion(TileMap* tilemap, Vec2i tile);

ecs_entity_t FindEntity(TileMap* tilemap, Vec2i start, ecs_id_t type);
