#include "TileMap.h"

#include "GameState.h"
#include "RenderUtils.h"

#include <raylib/src/raymath.h>
#include <math.h>

ZPL_TABLE_DEFINE(ChunkTable, chunk_, Chunk*);

internal Chunk* ChunkLoad(GameState* gameState, TileMap* tilemap, Vec2i coord);
internal void ChunkUnload(TileMap* tilemap, Chunk* chunk);
internal void ChunkUpdate(GameState* gameState, TileMap* tilemap, Chunk* chunk);
internal void ChunkGenerate(GameState* gameState, TileMap* tilemap, Chunk* chunk);
internal void ChunkBake(GameState* gameState, Chunk* chunk);

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
	size_t idx = (size_t)x + (size_t)y * CHUNK_SIZE;
	SASSERT(idx < CHUNK_AREA);
	return idx;
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

TileResult 
GetTileResult(TileMap* tilemap, Vec2i coord)
{
	TileResult result;
	result.Chunk = GetChunk(tilemap, coord);
	if (result.Chunk)
	{
		result.Index = GetLocalTileIdx(coord);
		result.Tile = &result.Chunk->TileArray[result.Index];
		SASSERT(result.Tile);
	}
	return result;
}

void 
SetTile(TileMap* tilemap, Vec2i coord, const Tile* tile)
{
	SASSERT(tilemap);
	SASSERT(tile);
	Chunk* chunk = GetChunk(tilemap, coord);
	if (chunk)
	{
		size_t idx = GetLocalTileIdx(coord);
		chunk->TileArray[idx] = *tile;
		chunk->IsDirty = true;
	}
}

void
TileMapInit(GameState* gameState, TileMap* tilemap)
{
	SASSERT(gameState);
	SASSERT(tilemap);

	zpl_array_init_reserve(tilemap->TexturePool, zpl_heap_allocator(), VIEW_DISTANCE_TOTAL_CHUNKS);
	zpl_array_resize(tilemap->TexturePool, VIEW_DISTANCE_TOTAL_CHUNKS);

	Vec2i reso = { VIEW_DISTANCE_TOTAL_CHUNKS * CHUNK_SIZE_PIXELS, CHUNK_SIZE_PIXELS };
	tilemap->TileMapTexture = LoadRenderTextureEx(reso, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8, false);

	for (int i = 0; i < zpl_array_count(tilemap->TexturePool); ++i)
	{
		tilemap->TexturePool[i] = { (float)i * CHUNK_SIZE_PIXELS, 0 };
	}

	chunk_init(&tilemap->Table, zpl_heap_allocator());
}

void 
TileMapFree(TileMap* tilemap)
{
	for (zpl_isize i = 0; i < zpl_array_count(tilemap->Table.entries); ++i)
	{
		Chunk* chunk = tilemap->Table.entries[i].value;
		ChunkUnload(tilemap, chunk);
	}
	chunk_destroy(&tilemap->Table);

	zpl_array_free(tilemap->TexturePool);

	UnloadRenderTexture(tilemap->TileMapTexture);
}

void 
TileMapUpdate(GameState* gameState, TileMap* tilemap)
{
	zpl_array(zpl_u64) unloadQueue;
	zpl_array_init_reserve(unloadQueue, gameState->FrameAllocator, 8);

	Vec2 position = gameState->Camera.target;
	position.x *= INVERSE_TILE_SIZE;
	position.y *= INVERSE_TILE_SIZE;

	if (tilemap->TileMapTexture.id < 1 || !IsTextureReady(tilemap->TileMapTexture.texture))
	{
		SERR("TileMapTexture is not loaded!");
		return;
	}

	BeginTextureMode(tilemap->TileMapTexture);

	// Loops over loaded chunks, Unloads out of range, handles chunks waiting for RenderTexture,
	// handles dirty chunks, and updates chunks.
	for (zpl_isize i = 0; i < zpl_array_count(tilemap->Table.entries); ++i)
	{
		zpl_u64 key = tilemap->Table.entries[i].key;
		Chunk* chunk = tilemap->Table.entries[i].value;
		float distance = Vector2DistanceSqr(chunk->CenterCoord, position);
		if (distance > VIEW_DISTANCE_SQR)
		{
			ChunkUnload(tilemap, chunk);
			zpl_array_append(unloadQueue, key);
		}
		else
		{
			if (chunk->IsWaitingToLoad && zpl_array_count(tilemap->TexturePool) > 0)
			{
				chunk->IsWaitingToLoad = false;
				chunk->IsDirty = true;

				size_t idx = (zpl_array_count(tilemap->TexturePool) - 1);
				chunk->TextureDrawPosition = tilemap->TexturePool[idx];
				
				zpl_array_pop(tilemap->TexturePool);
			}
			
			if (chunk->IsDirty && !chunk->IsWaitingToLoad)
			{
				ChunkBake(gameState, chunk);
			}

			ChunkUpdate(gameState, tilemap, chunk);
		}
	}

	EndTextureMode();

	// Remove unused chunks from map
	for (zpl_isize i = 0; i < zpl_array_count(unloadQueue); ++i)
	{
		chunk_remove(&tilemap->Table, unloadQueue[i]);
	}

	Vec2 chunkPos = position;
	chunkPos.x *= INVERSE_CHUNK_SIZE;
	chunkPos.y *= INVERSE_CHUNK_SIZE;

	// Checks for chunks needing to be loaded
	Vec2i start = Vec2ToVec2i(chunkPos) - Vec2i{ VIEW_RADIUS, VIEW_RADIUS };
	Vec2i end = Vec2ToVec2i(chunkPos) + Vec2i{ VIEW_RADIUS, VIEW_RADIUS };
	for (int y = start.y; y < end.y; ++y)
	{
		for (int x = start.x; x < end.x; ++x)
		{
			Vec2i coord = { x, y };
			Vec2 center;
			center.x = (float)coord.x * CHUNK_SIZE + ((float)CHUNK_SIZE / 2);
			center.y = (float)coord.y * CHUNK_SIZE + ((float)CHUNK_SIZE / 2);
			float distance = Vector2DistanceSqr(center, position);
			if (distance < VIEW_DISTANCE_SQR)
			{
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
}

void 
TileMapDraw(TileMap* tilemap, Rectangle screenRect)
{
	for (zpl_isize i = 0; i < zpl_array_count(tilemap->Table.entries); ++i)
	{
		Chunk* chunk = tilemap->Table.entries[i].value;
		if (CheckCollisionRecs(chunk->BoundingBox, screenRect))
		{
			Rectangle src =
			{
				chunk->TextureDrawPosition.x,
				chunk->TextureDrawPosition.y,
				CHUNK_SIZE_PIXELS,
				-CHUNK_SIZE_PIXELS
			};

			Rectangle dst =
			{
				chunk->BoundingBox.x,
				chunk->BoundingBox.y,
				CHUNK_SIZE_PIXELS,
				CHUNK_SIZE_PIXELS
			};
			DrawTexturePro(tilemap->TileMapTexture.texture, src, dst, {}, 0, WHITE);
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
	chunk->IsWaitingToLoad = true;
	chunk->IsDirty = true;

	ChunkGenerate(gameState, tilemap, chunk);

	SLOG_DEBUG("Chunk loaded. %s", FMT_VEC2I(chunk->Coord));

	return chunk;
}

internal void
ChunkUnload(TileMap* tilemap, Chunk* chunk)
{
	SASSERT(chunk);

	SLOG_DEBUG("Chunk unloaded. %s", FMT_VEC2I(chunk->Coord));

	if (!chunk->IsWaitingToLoad)
		zpl_array_append(tilemap->TexturePool, chunk->TextureDrawPosition);

	SFree(SPersistent, chunk);
}

internal void
ChunkUpdate(GameState* gameState, TileMap* tilemap, Chunk* chunk)
{

}

internal void
ChunkBake(GameState* gameState, Chunk* chunk)
{
	for (int y = 0; y < CHUNK_SIZE; ++y)
	{
		for (int x = 0; x < CHUNK_SIZE; ++x)
		{
			int localIdx = x + y * CHUNK_SIZE;

			TileRenderData* data = &chunk->TileRenderDataArray[localIdx];

			Rectangle src = { (float)data->x * TILE_SIZE, (float)data->y * TILE_SIZE, TILE_SIZE, TILE_SIZE };
			Rectangle dst = { (float)x * TILE_SIZE + chunk->TextureDrawPosition.x, (float)y * TILE_SIZE + chunk->TextureDrawPosition.y, TILE_SIZE, TILE_SIZE };
			DrawTexturePro(gameState->TileSpriteSheet, src, dst, {}, 0, WHITE);
		}
	}
	chunk->IsDirty = false;
}

internal void 
ChunkGenerate(GameState* gameState, TileMap* tilemap, Chunk* chunk)
{
	Vec2i startTile = chunk->Coord * Vec2i{ CHUNK_SIZE, CHUNK_SIZE };
	for (int y = 0; y < CHUNK_SIZE; ++y)
	{
		for (int x = 0; x < CHUNK_SIZE; ++x)
		{
			Vec2i tileCoord = Vec2i{ x, y } + startTile;

			int localIdx = x + y * CHUNK_SIZE;

			TileRenderData data;
			data.x = (uint16_t)zpl_random_range_i64(GetRandom(), 0, 16);
			data.y = (uint16_t)zpl_random_range_i64(GetRandom(), 0, 16);

			chunk->TileRenderDataArray[localIdx] = data;
			
			Tile* tile = &chunk->TileArray[localIdx];
			*tile = Tile{};
		}
	}
}