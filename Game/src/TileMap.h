#pragma once

#include "Core.h"

#include "Structures/BitArray.h"

struct GameState;

struct TileRenderData
{
	uint16_t x;
	uint16_t y;
};

enum TileFlags : uint8_t 
{
	TILE_FLAG_COLLISION	= Bit(0),
	TILE_FLAG_SOLID		= Bit(1),
	TILE_FLAG_LIQUID	= Bit(2),
	TILE_FLAG_SOFT		= Bit(3),
	TILE_FLAG_IS_HIDDEN = Bit(4),
};

struct Tile
{
	uint16_t TileId;
	BigFlags8 Flags;
};

struct Chunk
{
	Vec2i Coord;
	Rectangle BoundingBox;
	RenderTexture2D RenderTexture;
	Vector2 CenterCoord;
	bool IsDirty;
	bool IsGenerated;
	bool IsWaitingToLoad;
	TileRenderData TileRenderDataArray[CHUNK_AREA];
	Tile TileArray[CHUNK_AREA];
};

ZPL_TABLE_DECLARE(, ChunkTable, chunk_, Chunk*);

#define Vec2iHash(vec) zpl_fnv64a(&vec, sizeof(Vec2i));

struct TileMap
{
	ChunkTable Table;
	zpl_array(RenderTexture2D) TexturePool;
};

void TileMapInit(GameState* gameState, TileMap* tilemap);
void TileMapFree(TileMap* tilemap);
void TileMapUpdate(GameState* gameState, TileMap* tilemap);
void TileMapDraw(TileMap* tilemap, Rectangle screenRect);

Vec2i TileToChunk(Vec2i tile);

size_t GetLocalTileIdx(Vec2i tile);

Chunk* GetChunk(TileMap* tilemap, Vec2i tile);

Tile* GetTile(TileMap* tilemap, Vec2i tile);

struct TileResult
{
	Tile* Tile;
	Chunk* Chunk;
	size_t Index;
};
// Returns Tile*, Chunk* and index of tile in chunk's tile array
TileResult GetTileResult(TileMap* tilemap, Vec2i coord);

// Copies tile into chunks array.
void SetTile(TileMap* tilemap, Vec2i coord, const Tile* tile);
