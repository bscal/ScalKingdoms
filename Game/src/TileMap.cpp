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

internal zpl_isize ChunkThreadFunc(zpl_thread* thread);

// Internal thread safe chunk management functions
internal Chunk* InternalChunkLoad(TileMap* tilemap, Vec2i coord);
internal void InternalChunkUnload(TileMap* tilemap, Chunk* chunk);
internal void InternalChunkGenerate(TileMap* tilemap, Chunk* chunk);

// Main thread chunk function
internal void ChunkTick(GameState* gameState, Chunk* chunk); // Every tick

// Main thread chunk events
internal void OnChunkLoad(GameState* gameState, Chunk* chunk);
internal void OnChunkUnload(GameState* gameState, Chunk* chunk);
internal void OnChunkBake(GameState* gameState, Chunk* chunk); // When baked
internal void OnChunkUpdate(GameState* gameState, Chunk* chunk); // When a chunk update is triggered.

void
TileMapInit(GameState* gameState, TileMap* tilemap, Rectangle dimensions)
{
	SASSERT(gameState);
	SASSERT(tilemap);

	if (dimensions.width <= 0 || dimensions.height <= 0)
	{
		SError("TileMap dimensions need to be greater than 0");
		return;
	}

	tilemap->LastChunkCoord = Vec2i{ INT32_MAX, INT32_MAX };
	tilemap->Dimensions = dimensions;

	constexpr int VIEW_DISTANCE_TOTAL_CHUNKS = (VIEW_RADIUS * 2) * (VIEW_RADIUS * 2) + VIEW_RADIUS;

	zpl_array_init_reserve(tilemap->ChunkLoader.ChunkPool, zpl_heap_allocator(), VIEW_DISTANCE_TOTAL_CHUNKS);

	for (int i = 0; i < VIEW_DISTANCE_TOTAL_CHUNKS; ++i)
	{
		Chunk* chunk = (Chunk*)zpl_alloc(zpl_heap_allocator(), sizeof(Chunk));
		SClear(chunk, sizeof(Chunk));

		Vec2i reso = { CHUNK_SIZE_PIXELS, CHUNK_SIZE_PIXELS };
		chunk->RenderTexture = LoadRenderTextureEx(reso, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8, false);

		zpl_array_append(tilemap->ChunkLoader.ChunkPool, chunk);
	}

	HashMapTInitialize(&tilemap->ChunkMap, VIEW_DISTANCE_TOTAL_CHUNKS, Allocator::Arena);

	SInfoLog("Starting chunk thread...");

	// Init noise
	tilemap->ChunkLoader.Noise = fnlCreateState();
	tilemap->ChunkLoader.Noise.noise_type = FNL_NOISE_OPENSIMPLEX2;

	zpl_semaphore_init(&tilemap->ChunkLoader.Signal);
	zpl_array_init_reserve(tilemap->ChunkLoader.ChunksToAdd, zpl_heap_allocator(), MAX_CHUNKS_TO_PROCESS);
	zpl_array_init_reserve(tilemap->ChunkLoader.ChunkToRemove, zpl_heap_allocator(), MAX_CHUNKS_TO_PROCESS);

	zpl_thread_init_nowait(&tilemap->ChunkThread);
	zpl_thread_start(&tilemap->ChunkThread, ChunkThreadFunc, tilemap);

	SInfoLog("Chunk thread started!");

	SInfoLog("Tilemap Initialized!");
}

void
TileMapFree(TileMap* tilemap)
{
	SInfoLog("Waiting for chunk thread to shutdown...");
	// Waits for thread to shutdown;
	tilemap->ChunkLoader.ShouldShutdown = true;
	zpl_semaphore_post(&tilemap->ChunkLoader.Signal, 1);
	while (zpl_thread_is_running(&tilemap->ChunkThread))
	{
		zpl_sleep_ms(1);
	}
	zpl_thread_destroy(&tilemap->ChunkThread);
	zpl_semaphore_destroy(&tilemap->ChunkLoader.Signal);
	SInfoLog("Chunk thread shutdown!");

	for (u32 i = 0; i < tilemap->ChunkMap.Capacity; ++i)
	{
		if (tilemap->ChunkMap.Buckets[i].IsUsed)
		{
			Chunk* chunk = tilemap->ChunkMap.Buckets[i].Value;
			InternalChunkUnload(tilemap, chunk);
		}
	}
	HashMapTDestroy(&tilemap->ChunkMap);

	for (int i = 0; i < zpl_array_count(tilemap->ChunkLoader.ChunkPool); ++i)
	{
		UnloadRenderTexture(tilemap->ChunkLoader.ChunkPool[i]->RenderTexture);
		zpl_free(zpl_heap_allocator(), tilemap->ChunkLoader.ChunkPool[i]);
	}

	zpl_array_free(tilemap->ChunkLoader.ChunkPool);
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
			OnChunkUnload(gameState, chunkLoader->ChunkToRemove[i].ChunkPtr);
			HashMapTRemove(&tilemap->ChunkMap, &chunkLoader->ChunkToRemove[i].Key);
		}
		zpl_array_clear(chunkLoader->ChunkToRemove);

		// Add unused chunks from map
		for (zpl_isize i = 0; i < zpl_array_count(chunkLoader->ChunksToAdd); ++i)
		{
			HashMapTSet(&tilemap->ChunkMap, &chunkLoader->ChunksToAdd[i].Key, &chunkLoader->ChunksToAdd[i].ChunkPtr);
			OnChunkLoad(gameState, chunkLoader->ChunksToAdd[i].ChunkPtr);
		}
		zpl_array_clear(chunkLoader->ChunksToAdd);

		zpl_semaphore_post(&chunkLoader->Signal, 1);
	}

	// Loops over loaded chunks, Unloads out of range, handles chunks waiting for RenderTexture,
	// handles dirty chunks, and updates chunks.
	for (zpl_isize i = 0; i < tilemap->ChunkMap.Capacity; ++i)
	{
		if (!tilemap->ChunkMap.Buckets[i].IsUsed)
			continue;

		Chunk* chunk = tilemap->ChunkMap.Buckets[i].Value;

		if (chunk->BakeState != ChunkUpdateState::None)
		{
			OnChunkBake(gameState, chunk);
		}

		if (chunk->UpdateState != ChunkUpdateState::None)
		{
			OnChunkUpdate(gameState, chunk);
		}

		ChunkTick(gameState, chunk);
	}
}

void 
TileMapDraw(TileMap* tilemap, Rectangle screenRect)
{
	for (zpl_isize i = 0; i < tilemap->ChunkMap.Capacity; ++i)
	{
		if (!tilemap->ChunkMap.Buckets[i].IsUsed)
			continue;


		Chunk* chunk = tilemap->ChunkMap.Buckets[i].Value;
		if (CheckCollisionRecs(chunk->BoundingBox, screenRect))
		{
			Rectangle src =
			{
				0,
				0,
				CHUNK_SIZE_PIXELS,
				-CHUNK_SIZE_PIXELS
			};
			DrawTexturePro(chunk->RenderTexture.texture, src, chunk->BoundingBox, {}, 0, WHITE);
		}
	}
}

internal Chunk*
InternalChunkLoad(TileMap* tilemap, Vec2i coord)
{
	if (!IsChunkInBounds(tilemap, coord))
		return nullptr;

	if (zpl_array_count(tilemap->ChunkLoader.ChunkPool) <= 0)
		return nullptr;

	Chunk* chunk = zpl_array_back(tilemap->ChunkLoader.ChunkPool);
	SASSERT(chunk);
	zpl_array_pop(tilemap->ChunkLoader.ChunkPool);

	chunk->Coord = coord;
	chunk->BoundingBox.x = (float)coord.x * CHUNK_SIZE_PIXELS;
	chunk->BoundingBox.y = (float)coord.y * CHUNK_SIZE_PIXELS;
	chunk->BoundingBox.width = CHUNK_SIZE_PIXELS;
	chunk->BoundingBox.height = CHUNK_SIZE_PIXELS;
	chunk->CenterCoord.x = (float)coord.x * CHUNK_SIZE + ((float)CHUNK_SIZE / 2);
	chunk->CenterCoord.y = (float)coord.y * CHUNK_SIZE + ((float)CHUNK_SIZE / 2);
	chunk->BakeState = ChunkUpdateState::Self;
	chunk->UpdateState = ChunkUpdateState::SelfAndNeighbors;
	chunk->IsLoaded = true;

	InternalChunkGenerate(tilemap, chunk);

	SDebugLog("Chunk loaded. %s", FMT_VEC2I(chunk->Coord));

	return chunk;
}

internal void
InternalChunkUnload(TileMap* tilemap, Chunk* chunk)
{
	SASSERT(tilemap);
	SASSERT(chunk);

	chunk->IsLoaded = false;

	SDebugLog("Chunk unloaded. %s", FMT_VEC2I(chunk->Coord));

	zpl_array_append(tilemap->ChunkLoader.ChunkPool, chunk);
}

internal void 
ChunkTick(GameState* gameState, Chunk* chunk)
{
	if (chunk->BakeState != ChunkUpdateState::None || chunk->UpdateState != ChunkUpdateState::None)
	{
		bool bakeNeighbors = chunk->BakeState == ChunkUpdateState::SelfAndNeighbors;
		bool updateNeighbors = chunk->UpdateState == ChunkUpdateState::SelfAndNeighbors;
		if (bakeNeighbors || updateNeighbors)
		{
			for (size_t i = 0; i < ArrayLength(Vec2i_NEIGHTBORS); ++i)
			{
				Vec2i neighbor = chunk->Coord + Vec2i_NEIGHTBORS[i];
				Chunk* neighborChunk = GetChunkByCoordNoCache(&gameState->TileMap, neighbor);
				if (!neighborChunk)
					continue;
				
				if (bakeNeighbors && neighborChunk->BakeState == ChunkUpdateState::None)
					neighborChunk->BakeState = ChunkUpdateState::Self;
				
				if (updateNeighbors && neighborChunk->UpdateState == ChunkUpdateState::None)
					neighborChunk->UpdateState = ChunkUpdateState::Self;
			}
		}

		chunk->BakeState = ChunkUpdateState::None;
		chunk->UpdateState = ChunkUpdateState::None;
	}
}

internal void 
OnChunkLoad(GameState* gameState, Chunk* chunk)
{
}

internal void 
OnChunkUnload(GameState* gameState, Chunk* chunk)
{
	UnloadRegionPaths(&gameState->RegionState, chunk);
}

internal void 
OnChunkUpdate(GameState* gameState, Chunk* chunk)
{
	LoadRegionPaths(&gameState->RegionState, &gameState->TileMap, chunk);
}

internal void
OnChunkBake(GameState* gameState, Chunk* chunk)
{
	Texture2D* tileSpriteSheet = GetTileSheet();

	BeginTextureMode(chunk->RenderTexture);
	for (int y = 0; y < CHUNK_SIZE; ++y)
	{
		for (int x = 0; x < CHUNK_SIZE; ++x)
		{
			int localIdx = x + y * CHUNK_SIZE;

			float posX = (float)x * TILE_SIZE;
			float posY = (float)y * TILE_SIZE;

			Tile* tile = &chunk->TileArray[localIdx];

			Rectangle src = GetTileInfo(tile->BackgroundId)->Src;
			Rectangle dst = { posX, posY, TILE_SIZE, TILE_SIZE };
			
			DrawTexturePro(*tileSpriteSheet, src, dst, {}, 0, WHITE);

			if (tile->ForegroundId > 0)
			{
				src = GetTileInfo(tile->ForegroundId)->Src;
				DrawTexturePro(*tileSpriteSheet, src, dst, {}, 0, WHITE);
			}
		}
	}
	EndTextureMode();
}

internal void 
InternalChunkGenerate(TileMap* tilemap, Chunk* chunk)
{
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

			u16 bgId = 0;
			if (height >= .75f)
				bgId = Tiles::FIRE_WALL;
			else if (height >= -.1)
				bgId = Tiles::STONE;
			else
				bgId = Tiles::BLUE_STONE;

			Tile tile = {};
			tile.BackgroundId = bgId;

			TileInfo* tileInfo = GetTileInfo(bgId);
			tile.Flags = tileInfo->DefaultTileFlags;

			Tile fgTile = {};

			chunk->TileArray[localIdx] = tile;
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

Vec2i ChunkToTile(Vec2i chunk)
{
	Vec2i res;
	res.x = chunk.x * CHUNK_SIZE;
	res.y = chunk.y * CHUNK_SIZE;
	return res;
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
	if (chunkCoord == tilemap->LastChunkCoord)
		return tilemap->LastChunk;

	Chunk** chunkPtr = HashMapTGet(&tilemap->ChunkMap, &chunkCoord);
	if (chunkPtr)
	{
		tilemap->LastChunkCoord = chunkCoord;
		tilemap->LastChunk = *chunkPtr;
		return *chunkPtr;
	}
	else
	{
		return nullptr;
	}
}

Chunk* 
GetChunkByCoordNoCache(TileMap* tilemap, Vec2i chunkCoord)
{
	Chunk** chunkPtr = HashMapTGet(&tilemap->ChunkMap, &chunkCoord);
	if (chunkPtr)
		return *chunkPtr;
	else
		return nullptr;
}

Tile*
GetTile(TileMap* tilemap, Vec2i tileCoord)
{
	Chunk* chunk = GetChunk(tilemap, tileCoord);
	if (chunk)
	{
		size_t idx = GetLocalTileIdx(tileCoord);
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
SetTile(TileMap* tilemap, Vec2i coord, const Tile* tile)
{
	SASSERT(tilemap);
	SASSERT(tile);
	Chunk* chunk = GetChunk(tilemap, coord);
	if (chunk)
	{
		size_t idx = GetLocalTileIdx(coord);
		chunk->TileArray[idx] = *tile;
		chunk->BakeState = ChunkUpdateState::Self;
	}
}

void
SetTileId(Chunk* chunk, size_t idx, u16 tile, short layer)
{
	SASSERT(chunk);
	SASSERT(idx < CHUNK_AREA);
	SASSERT(layer >= 0);
	if (chunk)
	{
		chunk->BakeState = ChunkUpdateState::Self;

		switch (layer)
		{
		case(0):
		{
			chunk->TileArray[idx].BackgroundId = tile;
		} break;

		case(1):
		{
			chunk->TileArray[idx].ForegroundId = tile;
		} break;

		default:
			SWarn("SetTile using invalid layer."); break;
		}
	}
}

bool IsChunkInBounds(TileMap* tilemap, Vec2i coord)
{
	return (coord.x >= tilemap->Dimensions.x
		&& coord.y >= tilemap->Dimensions.y
		&& tilemap->Dimensions.width
		&& tilemap->Dimensions.height);
}

bool IsTileInBounds(TileMap* tilemap, Vec2i coord)
{
	float x = (float)coord.x * INVERSE_CHUNK_SIZE;
	float y = (float)coord.y * INVERSE_CHUNK_SIZE;
	return (x >= tilemap->Dimensions.x
		&& y >= tilemap->Dimensions.y
		&& x < tilemap->Dimensions.width
		&& y < tilemap->Dimensions.height);
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

		for (zpl_isize i = 0; i < tilemap->ChunkMap.Capacity; ++i)
		{
			if (!tilemap->ChunkMap.Buckets[i].IsUsed)
				continue;

			if (zpl_array_count(chunkLoader->ChunkToRemove) >= MAX_CHUNKS_TO_PROCESS)
				break;

			Vec2i key = tilemap->ChunkMap.Buckets[i].Key;
			Chunk* chunk = tilemap->ChunkMap.Buckets[i].Value;
			float distance = Vector2DistanceSqr(chunk->CenterCoord, position);
			if (distance > VIEW_DISTANCE_SQR)
			{
				InternalChunkUnload(tilemap, chunk);

				ChunkLoaderData data;
				data.ChunkPtr = chunk;
				data.Key = key;
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
					u32 idx = HashMapTFind(&tilemap->ChunkMap, &coord);
					if (idx == HashMapT<Vec2i, Chunk*>::NOT_FOUND)
					{
						ChunkLoaderData data;
						data.ChunkPtr = InternalChunkLoad(tilemap, coord);
						data.Key = coord;

						if (!data.ChunkPtr)
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

