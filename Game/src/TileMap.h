#pragma once

#include "Core.h"

struct GameState;

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
	zpl_list ChunkUnloadQueue;
};

void TileMapInit(GameState* gameState, TileMap* tilemap);
void TileMapFree(TileMap* tilemap);
void TileMapUpdate(GameState* gameState, TileMap* tilemap);

Vec2i TileToChunk(Vec2i tile);

size_t GetLocalTileIdx(Vec2i tile);

Chunk* GetChunk(TileMap* tilemap, Vec2i tile);

Tile* GetTile(TileMap* tilemap, Vec2i tile);