#include "Regions.h"

#include "GameState.h"
#include "Components.h"
#include "TileMap.h"

constexpr global_var int REGION_EDGE_LENGTH = REGION_SIZE - 1;

constexpr global_var Vec2i LOCAL_TR = { REGION_EDGE_LENGTH, 0 };
constexpr global_var Vec2i LOCAL_BL = { 0, REGION_EDGE_LENGTH };

void 
LoadRegionPaths(RegionPaths* regionPath, TileMap* tilemap, Chunk* chunk)
{
	SASSERT(regionPath);
	SASSERT(tilemap);
	SASSERT(chunk);

	ArrayList(Vec2i) tmpList = {};

	for (int i = 0; i < REGION_SIZE; ++i)
	{
		Vec2i tile = chunk->Coord + Vec2i{ i, 0 };
		Vec2i borderTile = tile + Vec2i_UP;
	}

}