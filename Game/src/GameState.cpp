#include "GameState.h"
#include "GameEntry.h"

#include "Memory.h"
#include "Tile.h"
#include "Sprite.h"
#include "Systems.h"
#include "Console.h"
#include "Debug.h"
#include "RenderUtils.h"

#define COMPONENT_DECLARATION
#include "Components.h"

#include <raylib/src/raymath.h>

global struct GameState State;

struct GameClient Client;

internal void GameRun();
internal void GameUpdate();
internal void GameLateUpdate();
internal void GameShutdown();
internal void InputUpdate();

internal void LoadAssets(GameState* gameState);
internal void UnloadAssets(GameState* gameState);

internal Vec2i ScreenToTile(Vec2 pos);

ECS_SYSTEM_DECLARE(DrawEntities);

internal void* RlMallocOverride(size_t sz)
{
	return MemArenaAlloc(&GetGameState()->GameMemory, sz);
}

internal void* RlReallocOverride(void* ptr, size_t sz)
{
	return MemArenaRealloc(&GetGameState()->GameMemory, ptr, sz);
}

internal void RlFreeOverride(void* ptr)
{
	if (!ptr)
		return;
	else 
		MemArenaFree(&GetGameState()->GameMemory, ptr);
}
#include "Structures/PoolArray.h"
int 
GameInitialize()
{
	size_t gameMemorySize = Megabytes(16);
	void* gameMemory = zpl_alloc_align(ZplAllocatorMalloc, gameMemorySize, 64);
	State.GameMemory = MemArenaCreate(gameMemory, gameMemorySize);

	zpl_arena_init_from_allocator(&State.FrameMemory, ZplAllocatorMalloc, Megabytes(16));

	zpl_affinity affinity;
	zpl_affinity_init(&affinity);

	//int threadCount = (affinity.thread_count > 4) ? 4 : 2;
	//zpl_jobs_init(&State.Jobs, ZplAllocatorArena, threadCount);
	//SInfoLog("[ Jobs ] Initialized Jobs with %d threads. CPU thread count: %d threads, %d cores"
	//	, threadCount, affinity.thread_count, affinity.core_count);

	Test();

	ConsoleInit();

	SetMallocCallBack(RlMallocOverride);
	SetReallocCallBack(RlReallocOverride);
	SetFreeCallBack(RlFreeOverride);

	InitWindow(WIDTH, HEIGHT, TITLE);
	SetTraceLogLevel(LOG_ALL);
	SetTargetFPS(60);

	State.Camera.zoom = 1.0f;
	State.Camera.offset = { (float)GetScreenWidth() / 2, (float)GetScreenHeight() / 2 };

	State.ScreenTexture = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());

	LoadAssets(&State);

	bool guiInitialized = InitializeGUI(&State, &State.AssetMgr.MainFont);
	SAssert(guiInitialized);

	HashMapTInitialize(&State.EntityMap, 64, Allocator::Arena);

	zpl_random_init(&State.Random);

	SpritesInitialize();

	TileMgrInitialize(&State.AssetMgr.TileSpriteSheet);

	TileMapInit(&State, &State.TileMap, { -8, -8, 8, 8 });

	// Entities
	State.World = ecs_init();

	ECS_COMPONENT_DEFINE(State.World, CTransform);
	ECS_COMPONENT_DEFINE(State.World, CRender);
	ECS_COMPONENT_DEFINE(State.World, CMove);
	
	ECS_TAG_DEFINE(State.World, GameObject);

	ECS_OBSERVER(State.World, MoveOnAdd, EcsOnAdd, CMove);
	ECS_OBSERVER(State.World, MoveOnRemove, EcsOnRemove, CMove);

	ECS_SYSTEM_DEFINE(State.World, DrawEntities, 0, CTransform, CRender);
	ECS_SYSTEM(State.World, MoveSystem, EcsOnUpdate, CTransform, CMove);

	Client.Player = SpawnCreature(&State, 0, { 0, 0 });

	RegionStateInit(&State.RegionState);
	PathfinderInit(&State.Pathfinder);
	PathfinderRegionsInit(&State.RegionPathfinder);

	Client.IsDebugMode = true;

	if (Client.IsDebugMode)
		SInfoLog("[ Game ] Running in DEBUG mode!");

	GameRun();

	ecs_fini(State.World);

	GameShutdown();

	return 0;
}

void
GameRun()
{
	while (!WindowShouldClose())
	{
		double start = GetTime();

		zpl_arena_snapshot arenaSnapshot = zpl_arena_snapshot_begin(&State.FrameMemory);

		// Update

		InputUpdate();

		UpdateGUI(&State);

		GameUpdate();

		ecs_progress(State.World, DeltaTime);

		// Draw

		Vector2 screenXY = GetScreenToWorld2D({ 0, 0 }, State.Camera);
		Rectangle screenRect;
		screenRect.x = screenXY.x;
		screenRect.y = screenXY.y;
		screenRect.width = (float)GetScreenWidth() / State.Camera.zoom;
		screenRect.height = (float)GetScreenHeight() / State.Camera.zoom;

		BeginTextureMode(State.ScreenTexture);
		ClearBackground(BLACK);
		BeginMode2D(State.Camera);

		TileMapDraw(&State.TileMap, screenRect);

		ecs_run(State.World, ecs_id(DrawEntities), DeltaTime, NULL);

		GameLateUpdate();

		EndMode2D();
		EndTextureMode();

		Client.UpdateTime = GetTime() - start;

		BeginDrawing();
		ClearBackground(BLACK);

		DrawTexturePro(State.ScreenTexture.texture
			, { 0, 0, (float)GetScreenWidth(), -(float)GetScreenHeight() }
			, { 0, 0, (float)GetScreenWidth(), (float)GetScreenHeight() }
			, {}, 0, WHITE);

		BeginMode2D(State.Camera);
		DrawRegions();
		EndMode2D();

		ConsoleDraw(&Client, &State.GUIState);
		DrawDebugWindow(&Client, &State, &State.GUIState);

		DrawGUI(&State);



		EndDrawing();

		zpl_arena_snapshot_end(arenaSnapshot);
	}
}

void GameUpdate()
{
	TileMapUpdate(&State, &State.TileMap);
}

void GameLateUpdate()
{
	//for (int i = 0; i < ArrayListCount(Client.DebugRegionsPath); ++i)
	//{
	//	int x = Client.DebugRegionsPath[i].x * REGION_SIZE * TILE_SIZE;
	//	int y = Client.DebugRegionsPath[i].y * REGION_SIZE * TILE_SIZE;
	//	DrawRectangle(x, y, REGION_SIZE * TILE_SIZE, REGION_SIZE * TILE_SIZE, Color{ 0, 100, 155, 155 });
	//}

	//Vector2 target = State.Camera.target;
	//DrawRectangleLines((int)(target.x - HALF_TILE_SIZE), (int)(target.y - HALF_TILE_SIZE), 16, 16, MAGENTA);

	//if (Client.DebugShowTilePaths)
	//{
		//Color closed = Color{ RED.r, RED.g, RED.b, 155 };
		//for (int i = 0; i < ArrayListCount(Client.PathfinderVisited); ++i)
		//{
		//	int x = Client.PathfinderVisited[i].x * TILE_SIZE;
		//	int y = Client.PathfinderVisited[i].y * TILE_SIZE;
		//	DrawRectangle(x, y, TILE_SIZE, TILE_SIZE, closed);
		//}
		//Color path = Color{ BLUE.r, BLUE.g, BLUE.b, 155 };
		//for (int i = 0; i < ArrayListCount(Client.DebugPathfinder); ++i)
		//{
		//	int x = Client.DebugPathfinder[i].x * TILE_SIZE;
		//	int y = Client.DebugPathfinder[i].y * TILE_SIZE;
		//	DrawRectangle(x, y, TILE_SIZE, TILE_SIZE, path);
		//}
	//}

	//chunk_map(&State.TileMap.Chunks, [](u64 key, Chunk* chunk) {
	//	constexpr int OFFSET = CHUNK_SIZE * TILE_SIZE;
	//	float rx = (float)chunk->Coord.x * OFFSET;
	//	float ry = (float)chunk->Coord.y * OFFSET;
	//	DrawRectangleLinesEx({ rx, ry, OFFSET, OFFSET }, 3.0f, ORANGE);
	//	});
	//HashMapKVForEach(&State.RegionState.RegionMap, [](void* hash, void* value, void* stack)
	//{
	//	constexpr int OFFSET = REGION_SIZE * TILE_SIZE;
	//	RegionPaths* region = (RegionPaths*)value;
	//	int rx = region->Pos.x * OFFSET;
	//	int ry = region->Pos.y * OFFSET;
	//	DrawRectangleLines(rx, ry, OFFSET, OFFSET, MAGENTA);
	//	DrawCircle(rx + OFFSET / 2, ry + OFFSET / 2 - REGION_SIZE, 2, (region->Sides[0]) ? GREEN : RED);
	//	DrawCircle(rx + OFFSET / 2 + REGION_SIZE, ry + OFFSET / 2, 2, (region->Sides[1]) ? GREEN : RED);
	//	DrawCircle(rx + OFFSET / 2, ry + OFFSET / 2 + REGION_SIZE, 2, (region->Sides[2]) ? GREEN : RED);
	//	DrawCircle(rx + OFFSET / 2 - REGION_SIZE, ry + OFFSET / 2, 2, (region->Sides[3]) ? GREEN : RED);
	//}, nullptr);

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
	State.Camera.target.x += movement.x;
	State.Camera.target.y += movement.y;
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
		Vec2i tile = ScreenToTile(GetMousePosition());
		SInfoLog("Tile: %s", FMT_VEC2I(tile));

		SList<Vec2i> next = PathFindArray(&State.Pathfinder, &State.TileMap, transform->TilePos, tile);

		if (next.IsAllocated())
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
		Vec2i tile = ScreenToTile(GetMousePosition());

		struct PathFindStack
		{
			ArrayList(Vec2i)* Path;
		};

		PathFindStack stack = {};
		stack.Path = &Client.DebugPathfinder;
		ecs_entity_t selectedEntity = Client.SelectedEntity;
		PathfindRegion(transform->TilePos, tile, &Client.MoveData);
		//MoveEntity(&State, selectedEntity, tile);
	}

	if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
	{
		Vec2i tile = ScreenToTile(GetMousePosition());

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

}

void
GameShutdown()
{
	UnloadRenderTexture(State.ScreenTexture);

	TileMapFree(&State.TileMap);

	UnloadAssets(&State);

	CloseWindow();
}

internal void 
LoadAssets(GameState* gameState)
{
	#define ASSET_DIR "Game/assets/"
	gameState->AssetMgr.TileSpriteSheet = LoadTexture(ASSET_DIR "16x16.bmp");
	gameState->AssetMgr.EntitySpriteSheet = LoadTexture(ASSET_DIR "SpriteSheet.png");
	gameState->AssetMgr.UIAtlas = SpriteAtlasLoad(ASSET_DIR, "UIAtlas.atlas");

	#define FONT_NAME "Silver"

	Rectangle fontRect = SpriteAtlasGet(&gameState->AssetMgr.UIAtlas, FONT_NAME);
	if (fontRect.width == 0 || fontRect.height == 0)
	{
		SAssertMsg(false, "Could not find font in ui atlas");
		return;
	}
	Vec2 offset = { fontRect.x, fontRect.y };
	gameState->AssetMgr.MainFont = LoadBMPFontFromTexture(ASSET_DIR FONT_NAME ".fnt", &gameState->AssetMgr.UIAtlas.Texture, offset);
	SAssert(gameState->AssetMgr.MainFont.texture.id > 0);
}

internal void 
UnloadAssets(GameState* gameState)
{
	UnloadTexture(gameState->AssetMgr.TileSpriteSheet);
	UnloadTexture(gameState->AssetMgr.EntitySpriteSheet);
	UnloadBMPFontData(&gameState->AssetMgr.MainFont);
	SpriteAtlasUnload(&gameState->AssetMgr.UIAtlas);
}

internal Vec2i 
ScreenToTile(Vec2 pos)
{
	Vec2 worldPos = GetScreenToWorld2D(pos, State.Camera);
	worldPos = Vector2Multiply(worldPos, { INVERSE_TILE_SIZE, INVERSE_TILE_SIZE });
	return Vec2ToVec2i(worldPos);
}

GameState* 
GetGameState()
{
	return &State;
}

GameClient* 
GetClient()
{
	return &Client;
}

zpl_allocator 
GetGameAllocator()
{
	return MemArenaAllocator(&State.GameMemory);
}

zpl_allocator
GetFrameAllocator()
{
	return zpl_arena_allocator(&State.FrameMemory);
}
