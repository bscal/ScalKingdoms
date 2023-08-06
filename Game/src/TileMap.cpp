#include "TileMap.h"

#include "GameState.h"
#include "RenderUtils.h"
#include "Tile.h"

#include <raylib/src/raymath.h>

#define FNL_IMPL
#include <FastNoiseLite/FastNoiseLite.h>

#include <math.h>

constexpr global_var int MAX_CHUNKS_TO_PROCESS = 12;
struct ChunkLoaderData
{
	Chunk* ChunkPtr;
	zpl_u64 Hash;
};

struct ChunkLoaderState
{
	fnl_state Noise;

	zpl_semaphore Signal;
	zpl_array(ChunkLoaderData) ChunksToAdd;
	zpl_array(ChunkLoaderData) ChunkToRemove;
	TileMap* Tilemap;
	Vec2 TargetPosition;
	bool ShouldShutdown;
	bool ShouldWork;
};

global_var ChunkLoaderState ChunkLoader;

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

	tilemap->Dimensions = dimensions;

	zpl_array_init_reserve(tilemap->TexturePool, zpl_heap_allocator(), VIEW_DISTANCE_TOTAL_CHUNKS);
	zpl_array_resize(tilemap->TexturePool, VIEW_DISTANCE_TOTAL_CHUNKS);

	Vec2i reso = { VIEW_DISTANCE_TOTAL_CHUNKS * CHUNK_SIZE_PIXELS, CHUNK_SIZE_PIXELS };
	tilemap->TileMapTexture = LoadRenderTextureEx(reso, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8, false);

	for (int i = 0; i < zpl_array_count(tilemap->TexturePool); ++i)
	{
		tilemap->TexturePool[i] = { (float)i * CHUNK_SIZE_PIXELS, 0 };
	}

	chunk_init(&tilemap->Chunks, zpl_heap_allocator());

	SLOG_INFO("Starting chunk thread...");

	// Init noise
	ChunkLoader.Noise = fnlCreateState();
	ChunkLoader.Noise.noise_type = FNL_NOISE_OPENSIMPLEX2;

	zpl_semaphore_init(&ChunkLoader.Signal);
	ChunkLoader.Tilemap = tilemap;
	zpl_array_init_reserve(ChunkLoader.ChunksToAdd, zpl_heap_allocator(), MAX_CHUNKS_TO_PROCESS);
	zpl_array_init_reserve(ChunkLoader.ChunkToRemove, zpl_heap_allocator(), MAX_CHUNKS_TO_PROCESS);

	zpl_thread_init_nowait(&tilemap->ChunkThread);
	zpl_thread_start(&tilemap->ChunkThread, ChunkThreadFunc, &ChunkLoader);

	SLOG_INFO("Chunk thread started!");

	SLOG_INFO("Tilemap Initialized!");
}

void
TileMapFree(TileMap* tilemap)
{
	SLOG_INFO("Waiting for chunk thread to shutdown...");
	// Waits for thread to shutdown;
	ChunkLoader.ShouldShutdown = true;
	zpl_semaphore_post(&ChunkLoader.Signal, 1);
	while (zpl_thread_is_running(&tilemap->ChunkThread))
	{
		zpl_sleep_ms(1);
	}
	zpl_thread_destroy(&tilemap->ChunkThread);
	zpl_semaphore_destroy(&ChunkLoader.Signal);
	SLOG_INFO("Chunk thread shutdown!");

	for (zpl_isize i = 0; i < zpl_array_count(tilemap->Chunks.entries); ++i)
	{
		Chunk* chunk = tilemap->Chunks.entries[i].value;
		// TODO: ? Chunk is unloaded here since it both free's the chunk and
		// appends the render texture to the pool. Though some chunk unloading
		// should be done on the thread. Like, serialization.
		ChunkUnload(tilemap, chunk);
	}
	chunk_destroy(&tilemap->Chunks);

	zpl_array_free(tilemap->TexturePool);

	UnloadRenderTexture(tilemap->TileMapTexture);
}

void 
TileMapUpdate(GameState* gameState, TileMap* tilemap)
{
	ChunkLoader.TargetPosition = gameState->Camera.target;

	// Note: Chunk thread will set to false when it finishes it work,
	// I think this is all safe. But I didn't wanted to make sure,
	// if the chunk thread took multiple frames to process, it
	// wouldn't be reading from it.
	if (!ChunkLoader.ShouldWork)
	{
		ChunkLoader.ShouldWork = true;
		// Remove unused chunks from map
		for (zpl_isize i = 0; i < zpl_array_count(ChunkLoader.ChunkToRemove); ++i)
		{
			chunk_remove(&tilemap->Chunks, ChunkLoader.ChunkToRemove[i].Hash);
			ChunkUnload(tilemap, ChunkLoader.ChunkToRemove[i].ChunkPtr);
		}
		zpl_array_clear(ChunkLoader.ChunkToRemove);

		// Remove unused chunks from map
		for (zpl_isize i = 0; i < zpl_array_count(ChunkLoader.ChunksToAdd); ++i)
		{
			chunk_set(&tilemap->Chunks, ChunkLoader.ChunksToAdd[i].Hash, ChunkLoader.ChunksToAdd[i].ChunkPtr);
		}
		zpl_array_clear(ChunkLoader.ChunksToAdd);

		zpl_semaphore_post(&ChunkLoader.Signal, 1);
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
		zpl_u64 key = tilemap->Chunks.entries[i].key;
		Chunk* chunk = tilemap->Chunks.entries[i].value;
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

	zpl_free(zpl_heap_allocator(), chunk);
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
		}
	}
	chunk->IsDirty = false;
}

internal void 
ChunkGenerate(GameState* gameState, TileMap* tilemap, Chunk* chunk)
{
	zpl_array(float) noiseValues;
	zpl_array_init_reserve(noiseValues, gameState->FrameAllocator, CHUNK_AREA);

	float startX = (float)chunk->Coord.x * CHUNK_SIZE;
	float startY = (float)chunk->Coord.y * CHUNK_SIZE;

	Vec2i startTile = chunk->Coord * Vec2i{ CHUNK_SIZE, CHUNK_SIZE };
	for (int y = 0; y < CHUNK_SIZE; ++y)
	{
		for (int x = 0; x < CHUNK_SIZE; ++x)
		{
			Vec2i tileCoord = Vec2i{ x, y } + startTile;

			int localIdx = x + y * CHUNK_SIZE;

			float height = fnlGetNoise2D(&ChunkLoader.Noise, startX + x, startY + y);

			Tile tile = {};
			if (height >= .25f)
			{
				tile.TileId = Tiles::STONE;
			}
			else
			{
				tile.TileId = Tiles::BLUE_STONE;
			}

			SetTile(chunk, localIdx, &tile, 0);
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
		SetTile(chunk, idx, tile, layer);
	}
}

void SetTile(Chunk* chunk, size_t idx, const Tile* tile, short layer)
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
	return (coord.x >= tilemap->Dimensions.x && coord.y >= tilemap->Dimensions.y
		&& coord.x < tilemap->Dimensions.width && coord.y < tilemap->Dimensions.height);
}

internal zpl_isize
ChunkThreadFunc(zpl_thread* thread)
{
	ChunkLoaderState* state = (ChunkLoaderState*)thread->user_data;
	while (true)
	{
		zpl_semaphore_wait(&state->Signal);

		if (state->ShouldShutdown)
			break;

		Vec2 position = Vector2Multiply(state->TargetPosition, { INVERSE_TILE_SIZE, INVERSE_TILE_SIZE });

		for (zpl_isize i = 0; i < zpl_array_count(state->Tilemap->Chunks.entries); ++i)
		{
			if (zpl_array_count(state->ChunkToRemove) >= MAX_CHUNKS_TO_PROCESS)
				break;

			zpl_u64 key = state->Tilemap->Chunks.entries[i].key;
			Chunk* chunk = state->Tilemap->Chunks.entries[i].value;
			float distance = Vector2DistanceSqr(chunk->CenterCoord, position);
			if (distance > VIEW_DISTANCE_SQR)
			{
				ChunkLoaderData data;
				data.ChunkPtr = chunk;
				data.Hash = key;
				zpl_array_append(state->ChunkToRemove, data);
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
				if (zpl_array_count(state->ChunksToAdd) >= MAX_CHUNKS_TO_PROCESS)
					break;

				Vec2i coord = { x, y };
				Vec2 center;
				center.x = (float)coord.x * CHUNK_SIZE + ((float)CHUNK_SIZE / 2);
				center.y = (float)coord.y * CHUNK_SIZE + ((float)CHUNK_SIZE / 2);
				float distance = Vector2DistanceSqr(center, position);
				if (distance < VIEW_DISTANCE_SQR)
				{
					zpl_u64 hash = Vec2iHash(coord);
					zpl_isize slot = chunk_slot(&state->Tilemap->Chunks, hash);
					if (slot == -1)
					{
						ChunkLoaderData data;
						data.ChunkPtr = ChunkLoad(GetGameState(), state->Tilemap, coord);
						data.Hash = hash;

						if (data.ChunkPtr == nullptr)
							continue;

						zpl_array_append(state->ChunksToAdd, data);
					}

				}
			}
		}

		state->ShouldWork = false;
	}
	return 0;
}

