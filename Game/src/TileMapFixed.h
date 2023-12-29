#pragma once

#include "Core.h"
#include "Tile.h"
#include "TileMap.h"
#include "Structures/StaticArray.h"
#include "Structures/HashMapT.h"
#include "Structures/SList.h"
#include "Lib/String.h"
#include "Lib/Jobs.h"

#include <FastNoiseLite/FastNoiseLite.h>

struct ChunkFixed
{
	RenderTexture2D RenderTexture;
	Rectangle BoundingBox;
	Vec2i Coord;
	ChunkUpdateState BakeState;
	ChunkUpdateState UpdateState;
	bool IsGenerated;
	bool IsLoaded; // TODO do we just remove this?
	Tile TileArray[CHUNK_AREA];
};

struct TileMapFixed
{
	SList<ChunkFixed> Chunks;
	fnl_state NoiseState;
	int LengthInChunks;
	int Seed;
};

void TileMapFixedCreate(TileMapFixed* tilemap, int length, int seed);
void TileMapFixedLoad(TileMapFixed* tilemap, GameState* state, String path);
void TileMapFixedUnload(TileMapFixed* tilemap, GameState* state);

void TileMapFixedUpdate(TileMapFixed* tilemap, GameState* state);
void TileMapFixedDraw(TileMapFixed* tilemap, Rectangle screenRect);

inline Vec2i
FixedChunkTileToChunk(Vec2i tile)
{
	Vec2i chunkCoord;
	chunkCoord.x = (int)floorf((float)tile.x / (float)CHUNK_SIZE);
	chunkCoord.y = (int)floorf((float)tile.y / (float)CHUNK_SIZE);
	return chunkCoord;
}

inline size_t
FixedChunkGetLocalTileIdx(Vec2i tile)
{
	int x = IntModNegative(tile.x, CHUNK_SIZE);
	int y = IntModNegative(tile.y, CHUNK_SIZE);
	size_t idx = (size_t)x + (size_t)y * CHUNK_SIZE;
	SAssert(idx < CHUNK_AREA);
	return idx;
}

inline bool 
FixedChunkIsInBounds(TileMapFixed* tilemap, Vec2i coord)
{
	return (coord.x >= 0
			&& coord.y >= 0
			&& coord.x < tilemap->LengthInChunks
			&& coord.y < tilemap->LengthInChunks);
}

inline bool 
FixedChunkIsTileInBounds(TileMapFixed* tilemap, Vec2i tile)
{
	int farestX = (tilemap->LengthInChunks + 1) * CHUNK_SIZE;
	int farestY = (tilemap->LengthInChunks + 1) * CHUNK_SIZE;
	return tile.x >= 0 && tile.y >= 0 && tile.x < farestX && tile.y < farestY;
}

inline ChunkFixed* 
FixedChunkGetByCoord(TileMapFixed* tilemap, Vec2i coord)
{
	SAssert(tilemap);
	if (FixedChunkIsInBounds(tilemap, coord))
	{
		i64 index = coord.x + coord.y * tilemap->LengthInChunks;
		SAssert(index < tilemap->Chunks.Capacity);
		return tilemap->Chunks.At(index);
	}
	else
	{
		return nullptr;
	}
}

inline ChunkFixed* 
FixedChunkGetByTile(TileMapFixed* tilemap, Vec2i tile)
{
	return FixedChunkGetByCoord(tilemap, FixedChunkTileToChunk(tile));
}

inline Tile*
GetTile(TileMapFixed* tilemap, Vec2i tile)
{
	ChunkFixed* chunk = FixedChunkGetByTile(tilemap, tile);
	if (chunk)
		return &chunk->TileArray[FixedChunkGetLocalTileIdx(tile)];
	else
		return nullptr;
}
