#pragma once

#include "Core.h"

#include "Structures/BitArray.h"

#include <FastNoiseLite/FastNoiseLite.h>

struct GameState;

constexpr global_var int LAYER_BACKGROUND = 0;
constexpr global_var int LAYER_FOREGROUND = 1;


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
	u16 BackgroundId;
	u16 ForegroundId;
	Flag8 Flags;
};

struct Chunk
{
	Vec2i Coord;
	Rectangle BoundingBox;
	Vec2 TextureDrawPosition;	// Start coords in the chunk texture
	Vec2 CenterCoord;
	bool IsDirty;				// Should Redraw chunk
	bool IsGenerated;
	bool IsLoaded;
	Tile TileArray[CHUNK_AREA];
};

struct ChunkLoaderData
{
	Chunk* ChunkPtr;
	zpl_u64 Hash;
};

struct ChunkLoaderState
{
	fnl_state Noise;

	zpl_semaphore Signal;
	zpl_array(Chunk*) ChunkPool;
	zpl_array(ChunkLoaderData) ChunksToAdd;
	zpl_array(ChunkLoaderData) ChunkToRemove;
	Vec2 TargetPosition;
	bool ShouldShutdown;
	bool ShouldWork;
};

ZPL_TABLE_DECLARE(, ChunkTable, chunk_, Chunk*);

struct TileMap
{
	RenderTexture2D TileMapTexture;
	ChunkTable Chunks;
	Rectangle Dimensions;
	zpl_thread ChunkThread;
	ChunkLoaderState ChunkLoader;
};

void TileMapInit(GameState* gameState, TileMap* tilemap, Rectangle dimensions);
void TileMapFree(TileMap* tilemap);
void TileMapUpdate(GameState* gameState, TileMap* tilemap);
void TileMapDraw(TileMap* tilemap, Rectangle screenRect);

Vec2i TileToChunk(Vec2i tile);

size_t GetLocalTileIdx(Vec2i tile);

Chunk* GetChunk(TileMap* tilemap, Vec2i tile);

Tile* GetTile(TileMap* tilemap, Vec2i tile);

struct TileFindResult
{
	Chunk* Chunk;
	size_t Index;
};
// Returns Chunk* and Index of coord in that chunk
TileFindResult FindTile(TileMap* tilemap, Vec2i coord);

// Copies tile into chunks array.
void SetTile(TileMap* tilemap, Vec2i coord, const Tile* tile);

void SetTileId(Chunk* chunk, size_t idx, u16 tile, short layer);

bool IsChunkInBounds(TileMap* tilemap, Vec2i coord);
bool IsTileInBounds(TileMap* tilemap, Vec2i coord);
