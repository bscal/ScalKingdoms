#include "GameState.h"

#include "Memory.h"
#include "Tile.h"
#include "Sprite.h"
#include "Systems.h"
#include "Console.h"
#include "Debug.h"
#include "RenderUtils.h"
#include "Components.h"
#include "Lighting.h"

#include "Lib/Jobs.h"

#include "raylib/src/rlgl.h"
#include <raylib/src/raymath.h>

internal void GameRun();
internal void GameUpdate();
internal void GameLateUpdate();
internal void GameShutdown();
internal void InputUpdate();

internal void LoadAssets(GameState* gameState);
internal void UnloadAssets(GameState* gameState);

internal void
RescaleGUI(GameState* gameState, GUIState* guiState, GUIScale scale);

ECS_SYSTEM_DECLARE(DrawEntities);

// MAYBE_FIXME what to do about these allocators
// raylib. I have to double check raylib allocations.
// Is mostly allocates to load things, and free at end of function
internal void* RlMallocOverride(size_t sz)
{
	return _aligned_malloc(sz, 16);
}

internal void* RlReallocOverride(void* ptr, size_t sz)
{
	return _aligned_realloc(ptr, sz, 16);
}

internal void RlFreeOverride(void* ptr)
{
	return _aligned_free(ptr);
}

int
GameInitialize()
{
	size_t permanentMemorySize = Megabytes(16);
	size_t gameMemorySize = Megabytes(16);
	size_t frameMemorySize = Megabytes(16);
	size_t totalMemory = permanentMemorySize + gameMemorySize + frameMemorySize;
	zpl_virtual_memory vm = zpl_vm_alloc(0, totalMemory);
	
	uintptr_t memoryOffset = (uintptr_t)vm.data;

	ArenaCreate(&State.GameArena, (void*)memoryOffset, permanentMemorySize);
	memoryOffset += permanentMemorySize;

	GeneralPurposeCreate(&State.GeneralPurposeMemory, (void*)memoryOffset, gameMemorySize);
	memoryOffset += gameMemorySize;

	ArenaCreate(&TransientState.TransientArena, (void*)memoryOffset, frameMemorySize);
	memoryOffset += frameMemorySize;

	SAssert(memoryOffset - vm.size == (size_t)vm.data);

	InitializeMemoryTracking();

	ArenaSnapshot tempMemoryInit = ArenaSnapshotBegin(&TransientState.TransientArena);

	PushMemoryIgnoreFree();

	JobsInitialize(7);

	ConsoleInit();

	SetMallocCallBack(RlMallocOverride);
	SetReallocCallBack(RlReallocOverride);
	SetFreeCallBack(RlFreeOverride);

	InitWindow(WIDTH, HEIGHT, TITLE);
	SetWindowState(FLAG_WINDOW_RESIZABLE);
	SetTraceLogLevel(LOG_ALL);
	SetTargetFPS(60);

	Client.GameResolution.x = GAME_WIDTH;
	Client.GameResolution.y = GAME_HEIGHT;

	State.Camera.zoom = 1.0f;
	State.Camera.offset = { (float)Client.GameResolution.x / 2, (float)Client.GameResolution.y / 2 };

	State.ScreenTexture = LoadRenderTexture(Client.GameResolution.x, Client.GameResolution.y);

	RescaleGUI(&State, &State.GUIState, GUIScale::SCALE_NORMAL);

	LoadAssets(&State);

	bool guiInitialized = InitializeGUI(&State, &State.AssetMgr.MainFont);
	SAssert(guiInitialized);

	HashMapTInitialize(&State.EntityMap, 4096, SAllocatorArena(&GetGameState()->GameArena));

	TileMgrInitialize(&State.AssetMgr.TileSpriteSheet);

	TileMapInit(&State, &State.TileMap, { -8, -8, 8, 8 });

	// Entities
	State.World = ecs_init();

	ECS_COMPONENT_DEFINE(State.World, CTransform);
	ECS_COMPONENT_DEFINE(State.World, CRender);
	ECS_COMPONENT_DEFINE(State.World, CMove);

	ECS_TAG_DEFINE(State.World, GameObject);

	//ECS_OBSERVER(State.World, MoveOnAdd, EcsOnAdd, CMove);
	//ECS_OBSERVER(State.World, MoveOnRemove, EcsOnRemove, CMove);

	ECS_SYSTEM_DEFINE(State.World, DrawEntities, 0, CTransform, CRender);
	ECS_SYSTEM(State.World, MoveSystem, EcsOnUpdate, CTransform, CMove);

	Client.Player = SpawnCreature(&State, 0, { 0, 0 });

	PathfinderInit(&State.Pathfinder);
	PathfinderRegionsInit(&State.RegionPathfinder);

	Client.IsDebugMode = true;

	if (Client.IsDebugMode)
		SInfoLog("[ Game ] Running in DEBUG mode!");

	TestSpareSet();

	PopMemoryIgnoreFree();

	static zpl_atomic32 val = {};
	for (int i = 0; i < 10; ++i)
	{
		JobHandle ctx = {};
		JobsExecute(&ctx, [](JobArgs* args)
					{
						zpl_atomic32* valPtr = (zpl_atomic32*)args->StackMemory;
						int v = zpl_atomic32_fetch_add(valPtr, 1);
						u32 tid = zpl_thread_current_id();
						SInfoLog("Thread %d, Printing %d", tid, v);
					}, &val);
	}

	LightMapInitialize();

	ArenaSnapshotEnd(tempMemoryInit);

	GameRun();

	GameShutdown();

	return 0;
}

void
GameRun()
{
	while (!WindowShouldClose())
	{
		double start = GetTime();

		ArenaSnapshot tempMemory = ArenaSnapshotBegin(&TransientState.TransientArena);

		// Update

		InputUpdate();

		UpdateGUI(&State);

		ConsoleDraw(&Client, &State.GUIState);
		DrawDebugWindow(&Client, &State, &State.GUIState);

		if (!State.IsGamePaused)
		{
			GameUpdate();
			ecs_progress(State.World, DeltaTime);
		}

		DrawErrorPopupWindow(&State);

		// Draw
		Vector2 screenXY = GetScreenToWorld2D({ 0, 0 }, State.Camera);
		Rectangle screenRect;
		screenRect.x = screenXY.x;
		screenRect.y = screenXY.y;
		screenRect.width = (float)GetScreenWidth() / State.Camera.zoom;
		screenRect.height = (float)GetScreenHeight() / State.Camera.zoom;

		BeginTextureMode(State.ScreenTexture);
		BeginMode2D(State.Camera);
		ClearBackground(BLACK);

		TileMapDraw(&State.TileMap, screenRect);

		ecs_run(State.World, ecs_id(DrawEntities), DeltaTime, NULL);

		GameLateUpdate();

		DrawRegions();

		DrawTexturePro(LightMapState.Texture.texture
					   , { 0, 0, LIGHTMAP_WIDTH, LIGHTMAP_WIDTH }
					   , { (float)LightMapState.Position.x * TILE_SIZE, (float)LightMapState.Position.y * TILE_SIZE , LIGHTMAP_WIDTH * TILE_SIZE, LIGHTMAP_WIDTH * TILE_SIZE }
		, {}, 0, WHITE);

		EndMode2D();
		EndTextureMode();

		BeginTextureMode(State.GUITexture);
		ClearBackground({});
		DrawGUI(&State);
		EndTextureMode();

		Client.UpdateTime = GetTime() - start;

		BeginDrawing();
		ClearBackground(BLACK);

		DrawTexturePro(State.ScreenTexture.texture
					   , { 0, 0, (float)Client.GameResolution.x, -(float)Client.GameResolution.y }
					   , { 0, 0, (float)GetScreenWidth(), (float)GetScreenHeight() }
		, {}, 0, WHITE);

		DrawTexturePro(State.GUITexture.texture
					   , { 0, 0, (float)State.GUITexture.texture.width, -(float)State.GUITexture.texture.height }
					   , { 0, 0, (float)GetScreenWidth(), (float)GetScreenHeight() }
		, {}, 0, WHITE);

		EndDrawing();

		ArenaSnapshotEnd(tempMemory);
	}
}

void GameUpdate()
{
	TileMapUpdate(&State, &State.TileMap);
	LightMapUpdate(&State);
}

void GameLateUpdate()
{
}

void InputUpdate()
{
	Vector2 movement = VEC2_ZERO;
	if (IsKeyDown(KEY_W))
	{
		movement.x += VEC2_UP.x * DeltaTime * 160;
		movement.y += VEC2_UP.y * DeltaTime * 160;
	}
	else if (IsKeyDown(KEY_S))
	{
		movement.x += VEC2_DOWN.x * DeltaTime * 160;
		movement.y += VEC2_DOWN.y * DeltaTime * 160;
	}
	if (IsKeyDown(KEY_A))
	{
		movement.x += VEC2_LEFT.x * DeltaTime * 160;
		movement.y += VEC2_LEFT.y * DeltaTime * 160;
	}
	else if (IsKeyDown(KEY_D))
	{
		movement.x += VEC2_RIGHT.x * DeltaTime * 160;
		movement.y += VEC2_RIGHT.y * DeltaTime * 160;
	}

	const CTransform* transform = ecs_get(State.World, Client.Player, CTransform);
	//State.Camera.target = Vector2Lerp(State.Camera.target, transform->Pos, 4.0f * DeltaTime);
	State.Camera.target = State.Camera.target + movement;
	if (IsKeyPressed(KEY_P))
	{
		State.Camera.target = transform->Pos;
	}

	float mouseWheel = GetMouseWheelMove();
	if (mouseWheel != 0)
	{
		constexpr float ZOOM_MAX = 5.0f;
		constexpr float ZOOM_MIN = .40f;
		constexpr float ZOOM_SPD = .20f;

		float newZoom = State.Camera.zoom + ZOOM_SPD * mouseWheel;

		if (newZoom > ZOOM_MAX)
			newZoom = ZOOM_MAX;
		else if (newZoom < ZOOM_MIN)
			newZoom = ZOOM_MIN;

		State.Camera.zoom = newZoom;
	}

	if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
	{
		Vec2i tile = ScreenToTile();
		SInfoLog("Tile: %s", FMT_VEC2I(tile));

		SList<Vec2i> next;
		{
			Timer();
			next = PathFindArray(&State.Pathfinder, &State.TileMap, transform->TilePos, tile);
		}

		if (next.Memory)
		{
			for (u32 i = 0; i < next.Count; ++i)
			{
				Tile t = NewTile(Tiles::WOOD_DOOR);
				SetTile(&State.TileMap, next.AtCopy(i), &t);
			}
		}
	}

	if (IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE))
	{
		Vec2i tile = ScreenToTile();
		ecs_entity_t selectedEntity = Client.SelectedEntity;
		MoveEntity(&State, selectedEntity, tile);
	}

	if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
	{
		Vec2i tile = ScreenToTile();

		ecs_entity_t* entity = HashMapTGet(&State.EntityMap, &tile);
		if (entity)
		{
			Client.SelectedEntity = *entity;
			const char* entityInfo = ecs_entity_str(State.World, *entity);
			SInfoLog("%s", entityInfo);
		}
	}

	if (IsKeyPressed(KEY_BACKSLASH))
	{
		Client.IsConsoleOpen = !Client.IsConsoleOpen;
	}
	if (IsKeyPressed(KEY_EQUAL))
	{
		Client.IsDebugWindowOpen = !Client.IsDebugWindowOpen;
	}

	if (IsKeyPressed(KEY_U))
	{
		Vec2i tile = ScreenToTile();
		if (IsTileInLightMap(tile))
		{
			Vec2i lightMapCoord = WorldToLightMap(tile);
			size_t idx = lightMapCoord.x + lightMapCoord.y * LIGHTMAP_WIDTH;
			LightMapState.Colors.At(idx)->r += .75f;
			LightMapState.Colors.At(idx)->g += 0;
			LightMapState.Colors.At(idx)->b += 0;
			LightMapState.Colors.At(idx)->a += 1;
		}
	}

}

void
GameShutdown()
{
	ecs_fini(State.World);

	//HashMapTDestroy(&State.EntityMap);

	UnloadRenderTexture(State.ScreenTexture);

	TileMapFree(&State.TileMap);

	UnloadAssets(&State);

	CloseWindow();

	ShutdownMemoryTracking();
}

internal void
LoadAssets(GameState* gameState)
{
#define SPRITE_MISS(name) SError("[ Assets ] Could not find sprite in spriteatlas: %s", name)

#define ASSET_DIR "Game/assets/"
	gameState->AssetMgr.TileSpriteSheet = LoadTexture(ASSET_DIR "16x16.bmp");
	gameState->AssetMgr.EntitySpriteSheet = LoadTexture(ASSET_DIR "SpriteSheet.png");
	gameState->AssetMgr.UIAtlas = SpriteAtlasLoad(ASSET_DIR, "UIAtlas.atlas");

#define FONT_NAME "Silver"

	SpriteAtlasGetResult fontResult = SpriteAtlasGet(&gameState->AssetMgr.UIAtlas, FONT_NAME);
	if (!fontResult.WasFound)
	{
		SPRITE_MISS(FONT_NAME);
	}

	Vec2 offset = { fontResult.Rect.x, fontResult.Rect.y };
	gameState->AssetMgr.MainFont = LoadBMPFontFromTexture(ASSET_DIR FONT_NAME ".fnt", &gameState->AssetMgr.UIAtlas.Texture, offset);
	SAssert(gameState->AssetMgr.MainFont.texture.id > 0);

	const char* SHAPES_TEXTURE_NAME = "TileSprite";
	SpriteAtlasGetResult tileResult = SpriteAtlasGet(&gameState->AssetMgr.UIAtlas, SHAPES_TEXTURE_NAME);
	if (!tileResult.WasFound)
	{
		SPRITE_MISS(SHAPES_TEXTURE_NAME);
	}
	else
	{
		SetShapesTexture(gameState->AssetMgr.UIAtlas.Texture, tileResult.Rect);
	}

}

internal void
UnloadAssets(GameState* gameState)
{
	UnloadTexture(gameState->AssetMgr.TileSpriteSheet);
	UnloadTexture(gameState->AssetMgr.EntitySpriteSheet);
	UnloadBMPFontData(&gameState->AssetMgr.MainFont);
	SpriteAtlasUnload(&gameState->AssetMgr.UIAtlas);
}

internal void
RescaleGUI(GameState* gameState, GUIState* guiState, GUIScale scale)
{
	UnloadRenderTexture(gameState->GUITexture);

	guiState->Scale.x = (float)GetScreenWidth() * GUIScaleResolutions[scale].x;
	guiState->Scale.y = (float)GetScreenHeight() * GUIScaleResolutions[scale].y;
	gameState->GUITexture = LoadRenderTexture((int)guiState->Scale.x, (int)guiState->Scale.y);
}
