#include "TileMap.h"

#include "Memory.h"
#include "GameState.h"
#include "RenderUtils.h"
#include "Tile.h"
#include "Utils.h"

#include <raylib/src/raymath.h>

#include <math.h>

#define Vec2iHash(vec) zpl_fnv64a(&vec, sizeof(Vec2i));

constexpr global_var int MAX_CHUNKS_TO_PROCESS = 12;

ZPL_TABLE_DEFINE(ChunkTable, chunk_, Chunk*);

internal Chunk* ChunkLoad(GameState* gameState, TileMap* tilemap, Vec2i coord);
internal void ChunkUnload(TileMap* tilemap, Chunk* chunk);
internal void ChunkUpdate(GameState* gameState, TileMap* tilemap, Chunk* chunk);
internal void ChunkGenerate(GameState* gameState, TileMap* tilemap, Chunk* chunk);
internal void ChunkBake(GameState* gameState, Chunk* chunk);

internal zpl_isize ChunkThreadFunc(zpl_thread* thread);

void
TileMapInit(GameState* gameState, TileMap* tilemap, Rectangle dimensions)
{
	SASSERT(gameState);
	SASSERT(tilemap);

	if (dimensions.width <= 0 || dimensions.height <= 0)
	{
		SERR("TileMap dimensions need to be greater than 0");
		return;
	}

	tilemap->Dimensions = dimensions;

	Vec2i reso = { VIEW_DISTANCE_TOTAL_CHUNKS * CHUNK_SIZE_PIXELS, CHUNK_SIZE_PIXELS };
	tilemap->TileMapTexture = LoadRenderTextureEx(reso, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8, false);

	zpl_array_init_reserve(tilemap->ChunkLoader.ChunkPool, zpl_heap_allocator(), VIEW_DISTANCE_TOTAL_CHUNKS);

	for (int i = 0; i < zpl_array_capacity(tilemap->ChunkLoader.ChunkPool); ++i)
	{
		Chunk* chunk = (Chunk*)zpl_alloc(zpl_heap_allocator(), sizeof(Chunk));
		SClear(chunk, sizeof(Chunk));

		chunk->TextureDrawPosition = { (float)i * CHUNK_SIZE_PIXELS, 0 };

		zpl_array_append(tilemap->ChunkLoader.ChunkPool, chunk);
	}

	chunk_init(&tilemap->Chunks, zpl_heap_allocator());

	SLOG_INFO("Starting chunk thread...");

	// Init noise
	tilemap->ChunkLoader.Noise = fnlCreateState();
	tilemap->ChunkLoader.Noise.noise_type = FNL_NOISE_OPENSIMPLEX2;

	zpl_semaphore_init(&tilemap->ChunkLoader.Signal);
	zpl_array_init_reserve(tilemap->ChunkLoader.ChunksToAdd, zpl_heap_allocator(), MAX_CHUNKS_TO_PROCESS);
	zpl_array_init_reserve(tilemap->ChunkLoader.ChunkToRemove, zpl_heap_allocator(), MAX_CHUNKS_TO_PROCESS);

	zpl_thread_init_nowait(&tilemap->ChunkThread);
	zpl_thread_start(&tilemap->ChunkThread, ChunkThreadFunc, tilemap);

	SLOG_INFO("Chunk thread started!");

	SLOG_INFO("Tilemap Initialized!");
}

void
TileMapFree(TileMap* tilemap)
{
	SLOG_INFO("Waiting for chunk thread to shutdown...");
	// Waits for thread to shutdown;
	tilemap->ChunkLoader.ShouldShutdown = true;
	zpl_semaphore_post(&tilemap->ChunkLoader.Signal, 1);
	while (zpl_thread_is_running(&tilemap->ChunkThread))
	{
		zpl_sleep_ms(1);
	}
	zpl_thread_destroy(&tilemap->ChunkThread);
	zpl_semaphore_destroy(&tilemap->ChunkLoader.Signal);
	SLOG_INFO("Chunk thread shutdown!");

	for (zpl_isize i = 0; i < zpl_array_count(tilemap->Chunks.entries); ++i)
	{
		Chunk* chunk = tilemap->Chunks.entries[i].value;
		ChunkUnload(tilemap, chunk);
		zpl_free(zpl_heap_allocator(), chunk);
	}
	chunk_destroy(&tilemap->Chunks);

	zpl_array_free(tilemap->ChunkLoader.ChunkPool);

	UnloadRenderTexture(tilemap->TileMapTexture);
}

void 
TileMapUpdate(GameState* gameState, TileMap* tilemap)
{
	ChunkLoaderState* chunkLoader = &tilemap->ChunkLoader;
	chunkLoader->TargetPosition = gameState->Camera.target;

	// Note: Chunk thread will set to false when it finishes it work,
	// I think this is all safe. But I didn't wanted to make sure,
	// if the chunk thread took multiple frames to process, it
	// wouldn't be reading from it.
	if (!chunkLoader->ShouldWork)
	{
		chunkLoader->ShouldWork = true;
		// Remove unused chunks from map
		for (zpl_isize i = 0; i < zpl_array_count(chunkLoader->ChunkToRemove); ++i)
		{
			chunk_remove(&tilemap->Chunks, chunkLoader->ChunkToRemove[i].Hash);
		}
		zpl_array_clear(chunkLoader->ChunkToRemove);

		// Remove unused chunks from map
		for (zpl_isize i = 0; i < zpl_array_count(chunkLoader->ChunksToAdd); ++i)
		{
			chunk_set(&tilemap->Chunks, chunkLoader->ChunksToAdd[i].Hash, chunkLoader->ChunksToAdd[i].ChunkPtr);
		}
		zpl_array_clear(chunkLoader->ChunksToAdd);

		zpl_semaphore_post(&chunkLoader->Signal, 1);
	}
	

	if (tilemap->TileMapTexture.id < 1 || !IsTextureReady(tilemap->TileMapTexture.texture))
	{
		SERR("TileMapTexture is not loaded!");
		return;
	}

	BeginTextureMode(tilemap->TileMapTexture);

	// Loops over loaded chunks, Unloads out of range, handles chunks waiting for RenderTexture,
	// handles dirty chunks, and updates chunks.
	for (zpl_isize i = 0; i < zpl_array_count(tilemap->Chunks.entries); ++i)
	{
		Chunk* chunk = tilemap->Chunks.entries[i].value;

		if (chunk->IsDirty)
		{
			ChunkBake(gameState, chunk);
		}
		else
		{
			ChunkUpdate(gameState, tilemap, chunk);
		}
	}

	EndTextureMode();
}

void 
TileMapDraw(TileMap* tilemap, Rectangle screenRect)
{
	for (zpl_isize i = 0; i < zpl_array_count(tilemap->Chunks.entries); ++i)
	{
		Chunk* chunk = tilemap->Chunks.entries[i].value;
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
	if (!IsChunkInBounds(tilemap, coord))
		return nullptr;

	Chunk* chunk = zpl_array_back(tilemap->ChunkLoader.ChunkPool);
	zpl_array_pop(tilemap->ChunkLoader.ChunkPool);

	SASSERT(chunk);

	chunk->Coord = coord;
	chunk->BoundingBox.x = (float)coord.x * CHUNK_SIZE_PIXELS;
	chunk->BoundingBox.y = (float)coord.y * CHUNK_SIZE_PIXELS;
	chunk->BoundingBox.width = CHUNK_SIZE_PIXELS;
	chunk->BoundingBox.height = CHUNK_SIZE_PIXELS;
	chunk->CenterCoord.x = (float)coord.x * CHUNK_SIZE + ((float)CHUNK_SIZE / 2);
	chunk->CenterCoord.y = (float)coord.y * CHUNK_SIZE + ((float)CHUNK_SIZE / 2);
	chunk->IsDirty = true;
	chunk->IsLoaded = true;

	ChunkGenerate(gameState, tilemap, chunk);

	SLOG_DEBUG("Chunk loaded. %s", FMT_VEC2I(chunk->Coord));

	return chunk;
}

internal void
ChunkUnload(TileMap* tilemap, Chunk* chunk)
{
	SASSERT(tilemap);
	SASSERT(chunk);

	chunk->IsLoaded = false;

	SLOG_DEBUG("Chunk unloaded. %s", FMT_VEC2I(chunk->Coord));

	zpl_array_append(tilemap->ChunkLoader.ChunkPool, chunk);
}

internal void
ChunkUpdate(GameState* gameState, TileMap* tilemap, Chunk* chunk)
{

}

internal void
ChunkBake(GameState* gameState, Chunk* chunk)
{
	Texture2D* tileSpriteSheet = GetTileSheet();
	for (int y = 0; y < CHUNK_SIZE; ++y)
	{
		for (int x = 0; x < CHUNK_SIZE; ++x)
		{
			int localIdx = x + y * CHUNK_SIZE;

			float posX = (float)x * TILE_SIZE + chunk->TextureDrawPosition.x;
			float posY = (float)y * TILE_SIZE + chunk->TextureDrawPosition.y;

			TileRenderData* data = &chunk->TileRenderDataArray[localIdx];

			Rectangle src = GetTileInfo(data->bgId)->Src;
			Rectangle dst = { posX, posY, TILE_SIZE, TILE_SIZE };
			
			DrawTexturePro(*tileSpriteSheet, src, dst, {}, 0, WHITE);

			if (data->fgId > 0)
			{
				src = GetTileInfo(data->fgId)->Src;
				DrawTexturePro(*tileSpriteSheet, src, dst, {}, 0, WHITE);
			}
		}
	}
	chunk->IsDirty = false;
}

internal void 
ChunkGenerate(GameState* gameState, TileMap* tilemap, Chunk* chunk)
{
	zpl_array(float) noiseValues;
	zpl_array_init_reserve(noiseValues, GetFrameAllocator(), CHUNK_AREA);

	float startX = (float)chunk->Coord.x * CHUNK_SIZE;
	float startY = (float)chunk->Coord.y * CHUNK_SIZE;

	Vec2i startTile = chunk->Coord * Vec2i{ CHUNK_SIZE, CHUNK_SIZE };
	for (int y = 0; y < CHUNK_SIZE; ++y)
	{
		for (int x = 0; x < CHUNK_SIZE; ++x)
		{
			Vec2i tileCoord = Vec2i{ x, y } + startTile;

			int localIdx = x + y * CHUNK_SIZE;

			float height = fnlGetNoise2D(&tilemap->ChunkLoader.Noise, startX + x, startY + y);

			Tile tile = {};
			if (height >= .25f)
			{
				tile.TileId = Tiles::STONE;
			}
			else
			{
				tile.TileId = Tiles::BLUE_STONE;
			}

			Tile fgTile = {};

			SetChunkTile(chunk, localIdx, &tile, 0);
			SetChunkTile(chunk, localIdx, &fgTile, 1);
		}
	}
}

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
	Chunk** chunkPtr = chunk_get(&tilemap->Chunks, hash);
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

TileFindResult
FindTile(TileMap* tilemap, Vec2i coord)
{
	TileFindResult result;
	result.Chunk = GetChunk(tilemap, coord);
	result.Index = (result.Chunk) ? GetLocalTileIdx(coord) : 0;
	return result;
}

void
SetTile(TileMap* tilemap, Vec2i coord, const Tile* tile, short layer)
{
	SASSERT(tilemap);
	SASSERT(tile);
	Chunk* chunk = GetChunk(tilemap, coord);
	if (chunk)
	{
		size_t idx = GetLocalTileIdx(coord);
		SetChunkTile(chunk, idx, tile, layer);
	}
}

void SetChunkTile(Chunk* chunk, size_t idx, const Tile* tile, short layer)
{
	SASSERT(chunk);
	SASSERT(idx < CHUNK_AREA);
	SASSERT(tile);
	SASSERT(layer >= 0);
	if (chunk)
	{
		chunk->TileArray[idx] = *tile;
		chunk->IsDirty = true;

		switch (layer)
		{
		case(0):
		{
			chunk->TileRenderDataArray[idx].bgId = tile->TileId;
		} break;

		case(1):
		{
			chunk->TileRenderDataArray[idx].fgId = tile->TileId;
		} break;

		default:
			SWARN("SetTile using invalid layer."); break;
		}
	}
	else
	{
		SERR("SetTile on unloaded chunk!");
	}
}

bool IsChunkInBounds(TileMap* tilemap, Vec2i coord)
{
	return (coord.x >= tilemap->Dimensions.x
		&& coord.y >= tilemap->Dimensions.y
		&& coord.x < tilemap->Dimensions.x + tilemap->Dimensions.width
		&& coord.y < tilemap->Dimensions.y + tilemap->Dimensions.height);
}

bool IsTileInBounds(TileMap* tilemap, Vec2i coord)
{
	float x = (float)coord.x * INVERSE_CHUNK_SIZE;
	float y = (float)coord.y * INVERSE_CHUNK_SIZE;
	return (x >= tilemap->Dimensions.x
		&& y >= tilemap->Dimensions.y
		&& x < tilemap->Dimensions.x + tilemap->Dimensions.width
		&& y < tilemap->Dimensions.y + tilemap->Dimensions.height);
}

internal zpl_isize
ChunkThreadFunc(zpl_thread* thread)
{
	TileMap* tilemap = (TileMap*)thread->user_data;
	ChunkLoaderState* chunkLoader = &tilemap->ChunkLoader;
	while (true)
	{
		zpl_semaphore_wait(&chunkLoader->Signal);

		if (chunkLoader->ShouldShutdown)
			break;

		Vec2 position = Vector2Multiply(chunkLoader->TargetPosition, { INVERSE_TILE_SIZE, INVERSE_TILE_SIZE });

		for (zpl_isize i = 0; i < zpl_array_count(tilemap->Chunks.entries); ++i)
		{
			if (zpl_array_count(chunkLoader->ChunkToRemove) >= MAX_CHUNKS_TO_PROCESS)
				break;

			zpl_u64 key = tilemap->Chunks.entries[i].key;
			Chunk* chunk = tilemap->Chunks.entries[i].value;
			float distance = Vector2DistanceSqr(chunk->CenterCoord, position);
			if (distance > VIEW_DISTANCE_SQR)
			{
				ChunkUnload(tilemap, chunk);

				ChunkLoaderData data;
				data.ChunkPtr = chunk;
				data.Hash = key;
				zpl_array_append(chunkLoader->ChunkToRemove, data);
			}
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
				if (zpl_array_count(chunkLoader->ChunksToAdd) >= MAX_CHUNKS_TO_PROCESS)
					break;

				Vec2i coord = { x, y };
				Vec2 center;
				center.x = (float)coord.x * CHUNK_SIZE + ((float)CHUNK_SIZE / 2);
				center.y = (float)coord.y * CHUNK_SIZE + ((float)CHUNK_SIZE / 2);
				float distance = Vector2DistanceSqr(center, position);
				if (distance < VIEW_DISTANCE_SQR)
				{
					zpl_u64 hash = Vec2iHash(coord);
					zpl_isize slot = chunk_slot(&tilemap->Chunks, hash);
					if (slot == -1)
					{
						ChunkLoaderData data;
						data.ChunkPtr = ChunkLoad(GetGameState(), tilemap, coord);
						data.Hash = hash;

						if (data.ChunkPtr == nullptr)
							continue;

						zpl_array_append(chunkLoader->ChunksToAdd, data);
					}

				}
			}
		}

		chunkLoader->ShouldWork = false;
	}
	return 0;
}

