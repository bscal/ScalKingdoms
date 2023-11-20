#include "TileMap.h"

#include "Components.h"
#include "Memory.h"
#include "GameState.h"
#include "RenderUtils.h"
#include "Tile.h"
#include "Utils.h"

#include <math.h>

// TODO
#pragma warning(disable: 4505)

internal void ChunkThreadFunc(JobArgs* args);

// Internal thread safe chunk management functions
internal Chunk* InternalChunkLoad(TileMap* tilemap, Vec2i coord);
internal void InternalChunkUnload(TileMap* tilemap, Chunk* chunk);
internal void InternalChunkGenerate(TileMap* tilemap, Chunk* chunk);

// Main thread chunk function
internal void ChunkTick(GameState* gameState, Chunk* chunk); // Every tick

// Main thread chunk events
internal void OnChunkLoad(GameState* gameState, Chunk* chunk);
internal void OnChunkUnload(GameState* gameState, TileMap* tilemap, Chunk* chunk);
internal void OnChunkBake(GameState* gameState, Chunk* chunk); // When baked
internal void OnChunkUpdate(GameState* gameState, Chunk* chunk); // When a chunk update is triggered.

internal Vec2i
TileToChunk(Vec2i tile)
{
	Vec2i chunkCoord;
	chunkCoord.x = (int)floorf((float)tile.x / (float)CHUNK_SIZE);
	chunkCoord.y = (int)floorf((float)tile.y / (float)CHUNK_SIZE);
	return chunkCoord;
}

internal size_t
GetLocalTileIdx(Vec2i tile)
{
	int x = IntModNegative(tile.x, CHUNK_SIZE);
	int y = IntModNegative(tile.y, CHUNK_SIZE);
	size_t idx = (size_t)x + (size_t)y * CHUNK_SIZE;
	SAssert(idx < CHUNK_AREA);
	return idx;
}

void
TileMapInit(GameState* gameState, TileMap* tilemap, Rectangle dimensions)
{
	SAssert(gameState);
	SAssert(tilemap);

	if (dimensions.width <= 0 || dimensions.height <= 0)
	{
		SError("TileMap dimensions need to be greater than 0");
		return;
	}

	tilemap->LastChunkCoord = Vec2i{ INT32_MAX, INT32_MAX };
	tilemap->Dimensions = dimensions;

	for (int i = 0; i < tilemap->ChunkLoader.ChunkPool.Capacity; ++i)
	{
		Chunk* chunk = (Chunk*)SAlloc(SAllocatorArena(&GetGameState()->GameArena), sizeof(Chunk));
		SAssert(chunk);

		SZero(chunk, sizeof(Chunk));

		Vec2i reso = { CHUNK_SIZE_PIXELS, CHUNK_SIZE_PIXELS };
		chunk->RenderTexture = LoadRenderTextureEx(reso, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8, false);

		chunk->Entities.Initialize(SAllocatorGeneral(), CHUNK_AREA);

		SAssert(chunk->RenderTexture.id != 0);

		tilemap->ChunkLoader.ChunkPool.Push(&chunk);
	}
	SAssert(tilemap->ChunkLoader.ChunkPool.Count == VIEW_DISTANCE_TOTAL_CHUNKS);

	HashMapTInitialize(&tilemap->ChunkMap, VIEW_DISTANCE_TOTAL_CHUNKS, SAllocatorArena(&GetGameState()->GameArena));

	// Init chunkloader
	tilemap->ChunkLoader.Tilemap = tilemap;
	tilemap->ChunkLoader.Noise = fnlCreateState();
	tilemap->ChunkLoader.Noise.noise_type = FNL_NOISE_OPENSIMPLEX2;

	SInfoLog("Tilemap Initialized!");
}

void
TileMapFree(TileMap* tilemap)
{
	SAssert(tilemap);

	for (u32 i = 0; i < tilemap->ChunkMap.Capacity; ++i)
	{
		if (tilemap->ChunkMap.Buckets[i].IsUsed)
		{
			Chunk* chunk = tilemap->ChunkMap.Buckets[i].Value;
			OnChunkUnload(GetGameState(), tilemap, chunk);
			InternalChunkUnload(tilemap, chunk);
		}
	}
	
	for (int i = 0; i < tilemap->ChunkLoader.ChunkPool.Count; ++i)
	{
		UnloadRenderTexture(tilemap->ChunkLoader.ChunkPool.Data[i]->RenderTexture);
	}
}

void 
TileMapUpdate(GameState* gameState, TileMap* tilemap)
{
	SAssert(gameState);
	SAssert(tilemap);

	ChunkLoaderState* chunkLoader = &tilemap->ChunkLoader;
	const CTransform* playerTransform = ecs_get(State.World, Client.Player, CTransform);
	chunkLoader->TargetPosition = playerTransform->Pos;

	if (!JobHandleIsBusy(&tilemap->ChunkLoaderJobHandle))
	{
		// Sync chunkloader

		// Remove unused chunks from map
		for (int i = 0; i < chunkLoader->ChunksToRemove.Count; ++i)
		{
			OnChunkUnload(gameState, tilemap, chunkLoader->ChunksToRemove.Data[i].ChunkPtr);
			HashMapTRemove(&tilemap->ChunkMap, &chunkLoader->ChunksToRemove.Data[i].Key);
		}
		chunkLoader->ChunksToRemove.Clear();

		// Add unused chunks from map
		for (int i = 0; i < chunkLoader->ChunksToAdd.Count; ++i)
		{
			HashMapTSet(&tilemap->ChunkMap, &chunkLoader->ChunksToAdd.Data[i].Key, &chunkLoader->ChunksToAdd.Data[i].ChunkPtr);
			OnChunkLoad(gameState, chunkLoader->ChunksToAdd.Data[i].ChunkPtr);
		}
		chunkLoader->ChunksToAdd.Clear();

		// Threaded chunk loading
		JobsExecute(&tilemap->ChunkLoaderJobHandle, ChunkThreadFunc, &tilemap->ChunkLoader);
	}

	// Loops over loaded chunks, Unloads out of range, handles chunks waiting for RenderTexture,
	// handles dirty chunks, and updates chunks.
	for (u32 i = 0; i < tilemap->ChunkMap.Capacity; ++i)
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
	SAssert(tilemap);
	for (u32 i = 0; i < tilemap->ChunkMap.Capacity; ++i)
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
			SAssert(chunk->RenderTexture.texture.id != 0);
			DrawTexturePro(chunk->RenderTexture.texture, src, chunk->BoundingBox, {}, 0, WHITE);
		}
	}
}

internal Chunk*
InternalChunkLoad(TileMap* tilemap, Vec2i coord)
{
	SAssert(tilemap);

	if (!IsChunkInBounds(tilemap, coord))
		return nullptr;

	if (tilemap->ChunkLoader.ChunkPool.Count <= 0)
		return nullptr;

	Chunk** chunkPtr = tilemap->ChunkLoader.ChunkPool.Last();
	SAssert(chunkPtr);
	Chunk* chunk = *chunkPtr;
	SAssert(*chunkPtr);

	tilemap->ChunkLoader.ChunkPool.Pop();

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
	SAssert(tilemap);
	SAssert(chunk);

	chunk->IsLoaded = false;

	SDebugLog("Chunk unloaded. %s", FMT_VEC2I(chunk->Coord));

	tilemap->ChunkLoader.ChunkPool.Push(&chunk);
}

internal void 
ChunkTick(GameState* gameState, Chunk* chunk)
{
	if (chunk->BakeState != ChunkUpdateState::None
		|| chunk->UpdateState != ChunkUpdateState::None)
	{
		bool bakeNeighbors = chunk->BakeState == ChunkUpdateState::SelfAndNeighbors;
		bool updateNeighbors = chunk->UpdateState == ChunkUpdateState::SelfAndNeighbors;
		if (bakeNeighbors || updateNeighbors)
		{
			for (size_t i = 0; i < ArrayLength(Vec2i_CARDINALS); ++i)
			{
				Vec2i neighbor = chunk->Coord + Vec2i_CARDINALS[i];
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
OnChunkUnload(GameState* gameState, TileMap* tilemap, Chunk* chunk)
{
	if (tilemap->LastChunkCoord == chunk->Coord)
		tilemap->LastChunkCoord = Vec2i_NULL;
	
	RegionUnload(chunk);
}

internal void 
OnChunkUpdate(GameState* gameState, Chunk* chunk)
{
	RegionLoad(&gameState->TileMap, chunk);
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

Chunk*
GetChunk(TileMap* tilemap, Vec2i tile)
{
	SAssert(tilemap);
	Vec2i chunkCoord = TileToChunk(tile);
	if (chunkCoord == tilemap->LastChunkCoord)
		return tilemap->LastChunk;

	Chunk** chunkPtr = HashMapTGet(&tilemap->ChunkMap, &chunkCoord);
	if (chunkPtr)
	{
		SAssert(*chunkPtr);
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
	SAssert(tilemap);
	Chunk** chunkPtr = HashMapTGet(&tilemap->ChunkMap, &chunkCoord);
	if (chunkPtr)
	{
		SAssert(*chunkPtr);
		return *chunkPtr;
	}
	else
	{
		return nullptr;
	}
}

Tile*
GetTile(TileMap* tilemap, Vec2i tileCoord)
{
	SAssert(tilemap);
	Chunk* chunk = GetChunk(tilemap, tileCoord);
	if (chunk)
	{
		size_t idx = GetLocalTileIdx(tileCoord);
		SAssert(idx < CHUNK_AREA);
		Tile* tile = &chunk->TileArray[idx];
		SAssert(tile);
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
	SAssert(tilemap);
	TileFindResult result;
	result.Chunk = GetChunk(tilemap, coord);
	result.Index = (result.Chunk) ? GetLocalTileIdx(coord) : 0;
	return result;
}

void
SetTile(TileMap* tilemap, Vec2i coord, const Tile* tile)
{
	SAssert(tilemap);
	SAssert(tile);
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
	SAssert(chunk);
	SAssert(idx < CHUNK_AREA);
	SAssert(layer >= 0);
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
		{
			SWarn("SetTile using invalid layer.");
			SAssertMsg(false, "SetTile using invalid layer.");
		} break;
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

void ProcessTileReachablity(TileMap* tilemap, Chunk* chunk)
{
	int idx = 0;
	for (int y = 0; y < CHUNK_SIZE; ++y)
	{
		for (int x = 0; x < CHUNK_SIZE; ++x)
		{
			

			++idx;
		}
	}
}

internal void
ChunkThreadFunc(JobArgs* args)
{
	ChunkLoaderState* chunkLoader = (ChunkLoaderState*)args->StackMemory;
	SAssert(chunkLoader);
	SAssert(chunkLoader->Tilemap);
	SAssert(chunkLoader->ChunksToAdd.Data);
	SAssert(chunkLoader->ChunksToRemove.Data);

	TileMap* tilemap = chunkLoader->Tilemap;
	if (!tilemap || !tilemap->ChunkMap.Buckets)
	{
		SAssertMsg(false, "Chunk thread, ChunkMap, is NULL");
		return;
	}

	Vec2 position = Vector2Multiply(chunkLoader->TargetPosition, { INVERSE_TILE_SIZE, INVERSE_TILE_SIZE });
	Vec2 chunkPos = Vector2Multiply(position, { INVERSE_CHUNK_SIZE, INVERSE_CHUNK_SIZE });

	// Checks for chunks needing to be loaded
	Vec2i start = Vec2ToVec2i(chunkPos) - Vec2i{ VIEW_RADIUS, VIEW_RADIUS };
	Vec2i end = Vec2ToVec2i(chunkPos) + Vec2i{ VIEW_RADIUS, VIEW_RADIUS };
	for (int y = start.y; y < end.y; ++y)
	{
		for (int x = start.x; x < end.x; ++x)
		{
			if (chunkLoader->ChunksToAdd.Count >= MAX_CHUNKS_TO_PROCESS || chunkLoader->ChunkPool.Count == 0)
				break;

			Vec2i coord = { x, y };

			if (!IsChunkInBounds(tilemap, coord))
				continue;

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
						break;

					chunkLoader->ChunksToAdd.Push(&data);
				}
			}
		}
	}

	for (u32 i = 0; i < tilemap->ChunkMap.Capacity; ++i)
	{
		if (chunkLoader->ChunksToRemove.Count >= MAX_CHUNKS_TO_PROCESS)
			break;
		
		if (!tilemap->ChunkMap.Buckets[i].IsUsed)
			continue;

		Vec2i key = tilemap->ChunkMap.Buckets[i].Key;
		Chunk* chunk = tilemap->ChunkMap.Buckets[i].Value;
		float distance = Vector2DistanceSqr(chunk->CenterCoord, position);
		if (distance > VIEW_DISTANCE_SQR)
		{
			InternalChunkUnload(tilemap, chunk);

			ChunkLoaderData data;
			data.ChunkPtr = chunk;
			data.Key = key;
			chunkLoader->ChunksToRemove.Push(&data);
		}
	}

}

