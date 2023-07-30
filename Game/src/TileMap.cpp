#include "TileMap.h"

#include "GameState.h"
#include "RenderUtils.h"

#include <raylib/src/raymath.h>
#include <math.h>

ZPL_TABLE_DEFINE(ChunkTable, chunk_, Chunk*);

internal Chunk* ChunkLoad(GameState* gameState, TileMap* tilemap, Vec2i coord);
internal void ChunkUnload(Chunk* chunk);
internal void ChunkUpdate(GameState* gameState, TileMap* tilemap, Chunk* chunk);
internal void ChunkGenerate(GameState* gameState, TileMap* tilemap, Chunk* chunk);
internal void ChunkBake(Chunk* chunk);

Vec2i 
TileToChunk(Vec2i tile)
{
	Vec2i chunkCoord;
	chunkCoord.x = (int)floorf((float)tile.x / (float)CHUNK_SIZE);
	chunkCoord.y = (int)floorf((float)tile.y / (float)CHUNK_SIZE);
	return chunkCoord;
}

size_t 
GetLocalTileIdx(Vec2i tile)
{
	int x = IntModNegative(tile.x, CHUNK_SIZE);
	int y = IntModNegative(tile.y, CHUNK_SIZE);
	int idx = x + y * CHUNK_SIZE;
	SASSERT(idx >= 0);
	return (size_t)idx;
}

Chunk* 
GetChunk(TileMap* tilemap, Vec2i tile)
{
	Vec2i chunkCoord = TileToChunk(tile);
	size_t hash = Vec2iHash(chunkCoord);
	Chunk** chunkPtr = chunk_get(&tilemap->Table, hash);
	if (chunkPtr)
	{
		return *chunkPtr;
	}
	else
	{
		return nullptr;
	}
}

Tile* 
GetTile(TileMap* tilemap, Vec2i tile)
{
	Chunk* chunk = GetChunk(tilemap, tile);
	if (chunk)
	{
		size_t idx = GetLocalTileIdx(tile);
		Tile* tile = &chunk->TileArray[idx];
		SASSERT(tile);
		return tile;
	}
	else
	{
		return nullptr;
	}

}

void
TileMapInit(GameState* gameState, TileMap* tilemap)
{
	SASSERT(gameState);
	SASSERT(tilemap);

	zpl_array_init_reserve(tilemap->TexturePool, zpl_heap_allocator(), VIEW_DISTANCE_TOTAL_CHUNKS);
	zpl_array_resize(tilemap->TexturePool, VIEW_DISTANCE_TOTAL_CHUNKS);

	for (int i = 0; i < zpl_array_count(tilemap->TexturePool); ++i)
	{
		Vec2i reso = { CHUNK_SIZE_PIXELS, CHUNK_SIZE_PIXELS };
		PixelFormat format = PIXELFORMAT_UNCOMPRESSED_R8G8B8;
		bool useDepth = false;
		tilemap->TexturePool[i] = LoadRenderTextureEx(reso, format, useDepth);
	}

	chunk_init(&tilemap->Table, zpl_heap_allocator());
}

void TileMapFree(TileMap* tilemap)
{
	chunk_map(&tilemap->Table, [](zpl_u64 key, Chunk* value)
		{
			ChunkUnload(value);
		});
	chunk_destroy(&tilemap->Table);


	for (int i = 0; i < zpl_array_count(tilemap->TexturePool); ++i)
	{
		UnloadRenderTexture(tilemap->TexturePool[i]);
	}
	zpl_array_free(tilemap->TexturePool);
}

void TileMapUpdate(GameState* gameState, TileMap* tilemap)
{
	zpl_array(zpl_u64) unloadQueue;
	zpl_array_init_reserve(unloadQueue, gameState->FrameAllocator, 8);

	Vec2 position = {};

	// Loops over loaded chunks, Unloads out of range, handles chunks waiting for RenderTexture,
	// handles dirty chunks, and updates chunks.
	for (zpl_isize i = 0; i < zpl_array_count(tilemap->Table.entries); ++i)
	{
		zpl_u64 key = tilemap->Table.entries[i].key;
		Chunk* chunk = tilemap->Table.entries[i].value;
		float distance = Vector2DistanceSqr(chunk->CenterCoord, position);
		if (distance > VIEW_DISTANCE_SQR)
		{
			ChunkUnload(chunk);
			zpl_array_append(unloadQueue, key);
		}
		else
		{
			if (chunk->IsWaitingToLoad && zpl_array_count(tilemap->TexturePool) > 0)
			{
				size_t idx = (zpl_array_count(tilemap->TexturePool) - 1);
				chunk->RenderTexture = tilemap->TexturePool[idx];
				zpl_array_pop(tilemap->TexturePool);
				chunk->IsDirty = true;
			}
			
			if (chunk->IsDirty)
			{
				ChunkBake(chunk);
			}

			ChunkUpdate(gameState, tilemap, chunk);
		}
	}

	// Remove unused chunks from map
	for (zpl_isize i = 0; i < zpl_array_count(unloadQueue); ++i)
	{
		chunk_remove(&tilemap->Table, unloadQueue[i]);
	}

	// Checks for chunks needing to be loaded
	Vec2i start = Vec2ToVec2i(position) - Vec2i{ VIEW_RADIUS, VIEW_RADIUS };
	Vec2i end = Vec2ToVec2i(position) + Vec2i{ VIEW_RADIUS, VIEW_RADIUS };
	for (int y = start.y; y < end.y; ++y)
	{
		for (int x = start.x; x < end.x; ++x)
		{
			Vec2i coord = { x, y };
			zpl_u64 hash = Vec2iHash(coord);
			Chunk** contains = chunk_get(&tilemap->Table, hash);
			if (!contains)
			{
				Chunk* chunk = ChunkLoad(gameState, tilemap, coord);
				chunk_set(&tilemap->Table, hash, chunk);
			}
		}
	}
}

internal Chunk*
ChunkLoad(GameState* gameState, TileMap* tilemap, Vec2i coord)
{
	Chunk* chunk = (Chunk*)SAlloc(SPersistent, sizeof(Chunk));
	SASSERT(chunk);
	SMemZero(chunk, sizeof(Chunk));

	chunk->Coord = coord;
	chunk->BoundingBox.x = (float)coord.x * CHUNK_SIZE_PIXELS;
	chunk->BoundingBox.y = (float)coord.y * CHUNK_SIZE_PIXELS;
	chunk->BoundingBox.width = CHUNK_SIZE_PIXELS;
	chunk->BoundingBox.height = CHUNK_SIZE_PIXELS;
	chunk->CenterCoord.x = (float)coord.x * CHUNK_SIZE + ((float)CHUNK_SIZE / 2);
	chunk->CenterCoord.y = (float)coord.y * CHUNK_SIZE + ((float)CHUNK_SIZE / 2);
	chunk->IsDirty = true;

	ChunkGenerate(gameState, tilemap, chunk);

	SLOG_DEBUG("Chunk loaded. %s", FMT_VEC2I(chunk->Coord));

	return chunk;
}

internal void
ChunkUnload(Chunk* chunk)
{
	SASSERT(chunk);

	SLOG_DEBUG("Chunk unloaded. %s", FMT_VEC2I(chunk->Coord));

	SFree(SPersistent, chunk);
}

internal void
ChunkUpdate(GameState* gameState, TileMap* tilemap, Chunk* chunk)
{

}

internal void
ChunkBake(Chunk* chunk)
{
	Vec2i startTile = chunk->Coord * Vec2i{ CHUNK_SIZE, CHUNK_SIZE };
	for (int y = 0; y < CHUNK_SIZE; ++y)
	{
		for (int x = 0; x < CHUNK_SIZE; ++x)
		{
			Vec2i tile = Vec2i{ x, y } + startTile;

			int localIdx = x + y * CHUNK_SIZE;
		}
	}
}

internal void 
ChunkGenerate(GameState* gameState, TileMap* tilemap, Chunk* chunk)
{
	Vec2i startTile = chunk->Coord * Vec2i{ CHUNK_SIZE, CHUNK_SIZE };
	for (int y = 0; y < CHUNK_SIZE; ++y)
	{
		for (int x = 0; x < CHUNK_SIZE; ++x)
		{
			Vec2i tile = Vec2i{ x, y } + startTile;

			int localIdx = x + y * CHUNK_SIZE;
		}
	}
}