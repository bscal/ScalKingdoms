#include "Core.h"

struct TileRenderData
{
	uint16_t X;
	uint16_t Y;
};

struct Tile
{

};

struct Chunk
{
	Vec2i Coord;
	TileRenderData TileRenderDataArray[CHUNK_AREA];
	Tile TileArray[CHUNK_AREA];
};

ZPL_TABLE(, ChunkTable, chunk_, Chunk);

#define Vec2iHash(vec) zpl_fnv64a(&vec, sizeof(Vec2i));

struct TileMap
{
	ChunkTable Table;
};


void TileMapInit(TileMap* tilemap)
{
	chunk_init(&tilemap->Table, zpl_heap_allocator());

	Vec2i coord = { 1, 1 };
	size_t hash = Vec2iHash(coord);
	
	Chunk chunk = {};
	chunk.Coord = coord;
	chunk_set(&tilemap->Table, hash, chunk);

	Vec2i coord2 = { -1, -1 };
	size_t hash2 = Vec2iHash(coord2);

	Chunk chunk2 = {};
	chunk2.Coord = coord2;
	chunk_set(&tilemap->Table, hash2, chunk2);


	Chunk* chunkPtr = chunk_get(&tilemap->Table, hash);

	zpl_printf("Printing! %d, %d", chunkPtr->Coord.x, chunkPtr->Coord.y);
}
