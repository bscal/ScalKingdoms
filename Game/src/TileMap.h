#pragma once

#include "Core.h"
#include "Game.h"
#include "Regions.h"
#include "Tile.h"
#include "Lib/Jobs.h"
#include "Structures/SList.h"
#include "Structures/BitArray.h"
#include "Structures/HashMapT.h"

#include <FastNoiseLite/FastNoiseLite.h>
#include <stdint.h>

struct GameState;

constant_var int LAYER_BACKGROUND	= 0;
constant_var int LAYER_FOREGROUND	= 1;
constant_var int LAYER_WALL			= 2;

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
	ItemStackHolder ItemHolder;
	Tile TileArray[CHUNK_AREA];
};

struct ChunkLoaderData
{
	Chunk* ChunkPtr;
	Vec2i Key;
};

struct ChunkLoaderState
{
	TileMap* Tilemap;
	Vec2 TargetPosition;
	SList<Chunk*> ChunkPool;
	SList<ChunkLoaderData> ChunksToAdd;
	SList<ChunkLoaderData> ChunkToRemove;
	fnl_state Noise;
};

struct TileMap
{
	Vec2i LastChunkCoord;
	Chunk* LastChunk;
	HashMapT<Vec2i, Chunk*> ChunkMap;
	Rectangle Dimensions;
	JobHandle ChunkLoaderJobHandle;
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
