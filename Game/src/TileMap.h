#pragma once

#include "Core.h"
#include "Regions.h"
#include "Structures/BitArray.h"
#include "Structures/HashMapKV.h"

#include <FastNoiseLite/FastNoiseLite.h>
#include <stdint.h>

struct GameState;

constexpr global_var int LAYER_BACKGROUND	= 0;
constexpr global_var int LAYER_FOREGROUND	= 1;
constexpr global_var int LAYER_WALL			= 2;

enum TileFlags : uint8_t 
{
	TILE_FLAG_COLLISION	= Bit(0),
	TILE_FLAG_LIQUID	= Bit(1),
	TILE_FLAG_IS_HIDDEN = Bit(2),
};

struct Tile
{
	u16 BackgroundId;
	u16 ForegroundId;
	Flag8 Flags;
};

enum class ChunkUpdateState : u8
{
	None				= 0,
	Self 				= Bit(0),
	SelfAndNeighbors 	= Bit(1),
};

struct Chunk
{
	RenderTexture2D RenderTexture;
	Rectangle BoundingBox;
	Vec2i Coord;
	Vec2 CenterCoord;
	ChunkUpdateState BakeState;
	ChunkUpdateState UpdateState;
	bool IsGenerated;
	bool IsLoaded; // TODO do we just remove this?
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
	ChunkTable Chunks;
	Vec2i LastChunkCoord;
	Chunk* LastChunk;
	Rectangle Dimensions;
	zpl_thread ChunkThread;
	ChunkLoaderState ChunkLoader;
};

void TileMapInit(GameState* gameState, TileMap* tilemap, Rectangle dimensions);
void TileMapFree(TileMap* tilemap);
void TileMapUpdate(GameState* gameState, TileMap* tilemap);
void TileMapDraw(TileMap* tilemap, Rectangle screenRect);

Vec2i TileToChunk(Vec2i tile);

Vec2i ChunkToTile(Vec2i chunk);

size_t GetLocalTileIdx(Vec2i tile);

Chunk* GetChunk(TileMap* tilemap, Vec2i tile);
Chunk* GetChunkByCoordNoCache(TileMap* tilemap, Vec2i chunkCoord);

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
