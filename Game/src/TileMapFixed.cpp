#include "TileMapFixed.h"

#include "GameState.h"
#include "RenderUtils.h"

internal void 
InternalChunkGenerate(TileMapFixed* tilemap, ChunkFixed* chunk)
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

			float height = fnlGetNoise2D(&tilemap->NoiseState, startX + x, startY + y);

			u16 bgId = 0;
			if (height >= .75f)
				bgId = Tiles::FIRE_WALL;
			else if (height >= -.1)
				bgId = Tiles::STONE;
			else
				bgId = Tiles::BLUE_STONE;

			Tile tile = {};
			tile.BackgroundId = bgId;

			TileDef* tileInfo = GetTileDef(bgId);
			tile.Flags = tileInfo->DefaultTileFlags;

			Tile fgTile = {};

			chunk->TileArray[localIdx] = tile;
		}
	}
}

void TileMapFixedCreate(TileMapFixed* tilemap, int length, int seed)
{
	SAssert(!tilemap->Chunks.Memory);
	SAssert(length > 0);

	tilemap->LengthInChunks = length;
	tilemap->Seed = seed;
	
	tilemap->Chunks.Reserve(SAllocatorGeneral(), length * length);
	tilemap->Chunks.EnsureSize(SAllocatorGeneral(), length * length);
	
	tilemap->NoiseState = fnlCreateState();
	tilemap->NoiseState.noise_type = FNL_NOISE_OPENSIMPLEX2;

	int size = TILE_SIZE * CHUNK_SIZE;
	for (u32 i = 0; i < tilemap->Chunks.Count; ++i)
	{
		ChunkFixed* chunk = tilemap->Chunks.At(i);
		*chunk = {};
		chunk->Coord.x = (int)i / length;
		chunk->Coord.y = (int)i % length;
		chunk->BoundingBox.x = (float)chunk->Coord.x * CHUNK_SIZE_PIXELS;
		chunk->BoundingBox.y = (float)chunk->Coord.y * CHUNK_SIZE_PIXELS;
		chunk->BoundingBox.width = CHUNK_SIZE_PIXELS;
		chunk->BoundingBox.height = CHUNK_SIZE_PIXELS;
		chunk->RenderTexture = LoadRenderTextureEx({ size, size}, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8, false);
		chunk->BakeState = ChunkUpdateState::Self;
		chunk->UpdateState = ChunkUpdateState::SelfAndNeighbors;
		chunk->IsLoaded = true;

		InternalChunkGenerate(tilemap, chunk);
	}
}

void TileMapFixedLoad(TileMapFixed* tilemap, GameState* state, String path)
{

}

void TileMapFixedUnload(TileMapFixed* tilemap, GameState* state)
{
	for (u32 i = 0; i < tilemap->Chunks.Count; ++i)
	{
		UnloadRenderTexture(tilemap->Chunks.Memory[i].RenderTexture);
	}
	tilemap->Chunks.Free();
}

internal void 
ChunkTick(GameState* gameState, TileMapFixed* tilemap, ChunkFixed* chunk)
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
				ChunkFixed* neighborChunk = FixedChunkGetByCoord(tilemap, neighbor);
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
OnChunkUpdate(GameState* gameState, ChunkFixed* chunk)
{
	RegionLoad(&gameState->MainTileMap, chunk->Coord);
}

internal void
OnChunkBake(GameState* gameState, ChunkFixed* chunk)
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

			Rectangle src = GetTileDef(tile->BackgroundId)->SpriteSheetRect;
			Rectangle dst = { posX, posY, TILE_SIZE, TILE_SIZE };
			
			DrawTexturePro(*tileSpriteSheet, src, dst, {}, 0, WHITE);

			if (tile->ForegroundId > 0)
			{
				src = GetTileDef(tile->ForegroundId)->SpriteSheetRect;
				DrawTexturePro(*tileSpriteSheet, src, dst, {}, 0, WHITE);
			}
		}
	}
	EndTextureMode();
}

void TileMapFixedUpdate(TileMapFixed* tilemap, GameState* state)
{
	for (u32 i = 0; i < tilemap->Chunks.Count; ++i)
	{
		ChunkFixed* chunk = tilemap->Chunks.At(i);

		if (chunk->BakeState != ChunkUpdateState::None)
		{
			OnChunkBake(state, chunk);
		}

		if (chunk->UpdateState != ChunkUpdateState::None)
		{
			OnChunkUpdate(state, chunk);
		}

		ChunkTick(state, tilemap, chunk);
	}
}

void TileMapFixedDraw(TileMapFixed* tilemap, Rectangle screenRect)
{
	for (u32 i = 0; i < tilemap->Chunks.Count; ++i)
	{
		ChunkFixed* chunk = tilemap->Chunks.At(i);
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
