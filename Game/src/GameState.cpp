#include "GameState.h"
#include "GameEntry.h"

#include "Memory.h"
#include "Tile.h"
#include "Sprite.h"
#include "Systems.h"

#define COMPONENT_DECLARATION
#include "Components.h"

#include <raylib/src/raymath.h>

global_var struct GameState State;
global_var struct GameClient Client;

internal void GameRun();
internal void GameUpdate();
internal void GameShutdown();
internal void InputUpdate();

internal void LoadAssets(GameState* gameState);
internal void UnloadAssets(GameState* gameState);

internal Vec2i ScreenToTile(Vec2 pos);

int 
GameInitialize()
{
	zpl_arena_init_from_allocator(&State.GameMemory, zpl_heap_allocator(), Megabytes(8));

	InitWindow(WIDTH, HEIGHT, TITLE);
	SetTraceLogLevel(LOG_ALL);
	SetTargetFPS(60);


	State.Camera.zoom = 1.0f;
	State.Camera.offset = { (float)GetScreenWidth() / 2, (float)GetScreenHeight() / 2 };

	LoadAssets(&State);
	
	InitializeGUI(&State, State.AssetMgr.MainFont);

	HashMapInitialize(&State.EntityMap, sizeof(ecs_entity_t), 64, ALLOCATOR_HEAP);

	zpl_random_init(&State.Random);
	Sprite* sprite = SpriteGet(Sprites::PLAYER);

	TileMgrInitialize(&State.AssetMgr.TileSpriteSheet);

	TileMapInit(&State, &State.TileMap, { 0, 0, 4, 4 });

	// Entities
	State.World = ecs_init();

	ECS_COMPONENT_DEFINE(State.World, CTransform);
	ECS_COMPONENT(State.World, CRender);
	ECS_COMPONENT_DEFINE(State.World, CMove);

	ECS_SYSTEM(State.World, DrawEntities, EcsOnUpdate, CTransform, CRender);
	ECS_SYSTEM(State.World, MoveSystem, EcsOnUpdate, CTransform, CMove);

	Client.Player = ecs_new_id(State.World);
	ecs_add(State.World, Client.Player, CTransform);
	ecs_add(State.World, Client.Player, CRender);
	ecs_add(State.World, Client.Player, CMove);
	
	ecs_set_ex(State.World, Client.Player, CTransform, {});

	CRender render = {};
	render.Color = WHITE;
	render.Width = TILE_SIZE;
	render.Height = TILE_SIZE;
	ecs_set(State.World, Client.Player, CRender, render);

	CMove c = {};
	ecs_set(State.World, Client.Player, CMove, c);

	Vec2i pos = Vec2i{ 0, 0 };
	u32 hash = HashTile(pos);
	HashMapSet(&State.EntityMap, hash, Client.Player, ecs_entity_t);

	PathfinderInit(&State.Pathfinder);

	GameRun();

	GameShutdown();

	return 0;
}

void
GameRun()
{
	while (!WindowShouldClose())
	{
		zpl_arena_snapshot arenaSnapshot = zpl_arena_snapshot_begin(&State.GameMemory);

		// Update

		InputUpdate();

		GameUpdate();

		// Draw

		Vector2 screenXY = GetScreenToWorld2D({ 0, 0 }, State.Camera);
		Rectangle screenRect;
		screenRect.x = screenXY.x;
		screenRect.y = screenXY.y;
		screenRect.width = (float)GetScreenWidth() / State.Camera.zoom;
		screenRect.height = (float)GetScreenHeight() / State.Camera.zoom;

		BeginDrawing();
		ClearBackground(BLACK);
		BeginMode2D(State.Camera);

		TileMapDraw(&State.TileMap, screenRect);

		ecs_progress(State.World, DeltaTime);

		Vector2 target = State.Camera.target;
		DrawRectangleLines((int)target.x - 8, (int)target.y - 8, 16, 16, MAGENTA);

		EndMode2D();

		EndDrawing();

		zpl_arena_snapshot_end(arenaSnapshot);
	}
}

void GameUpdate()
{
	TileMapUpdate(&State, &State.TileMap);
}

void InputUpdate()
{
	Vector2 movement = VEC2_ZERO;
	int8_t moveX = 0;
	int8_t moveY = 0;

	if (IsKeyDown(KEY_W))
	{
		movement.x += VEC2_UP.x * DeltaTime * 160;
		movement.y += VEC2_UP.y * DeltaTime * 160;
		moveY = -1;
	}
	else if (IsKeyDown(KEY_S))
	{
		movement.x += VEC2_DOWN.x * DeltaTime * 160;
		movement.y += VEC2_DOWN.y * DeltaTime * 160;
		moveY = 1;
	}

	if (IsKeyDown(KEY_A))
	{
		movement.x += VEC2_LEFT.x * DeltaTime * 160;
		movement.y += VEC2_LEFT.y * DeltaTime * 160;
		moveX = -1;
	}
	else if (IsKeyDown(KEY_D))
	{
		movement.x += VEC2_RIGHT.x * DeltaTime * 160;
		movement.y += VEC2_RIGHT.y * DeltaTime * 160;
		moveX = 1;
	}
	
	//CMove* move = (CMove*)ecs_get(State.World, Client.Player, CMove);

	const CTransform* transform = ecs_get(State.World, Client.Player, CTransform);
	State.Camera.target = transform->Pos;

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
		SLOG_INFO("Tile: %s", FMT_VEC2I(tile));

		const CTransform* transform = ecs_get(State.World, Client.Player, CTransform);

		SList<Vec2i> next = PathFindArray(&State.Pathfinder, &State.TileMap, transform->TilePos, tile);

		if (next.IsAllocated())
		{
			for (u32 i = 0; i < next.Count; ++i)
			{
				Tile t = NewTile(Tiles::WOOD_DOOR);
				SetTile(&State.TileMap, next[i], &t);
			}
		}
	}

	if (IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE))
	{
		Vec2i tile = ScreenToTile(GetMousePosition());
		ecs_entity_t selectedEntity = Client.SelectedEntity;
		MoveEntity(&State, selectedEntity, tile);
	}

	if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
	{
		Vec2i tile = ScreenToTile(GetMousePosition());

		u32 hash = zpl_fnv32a(&tile, sizeof(Vec2i));
		ecs_entity_t* entity = HashMapGet(&State.EntityMap, hash, ecs_entity_t);
		if (entity)
		{
			Client.SelectedEntity = *entity;
			const char* entityInfo = ecs_entity_str(State.World, *entity);
			SLOG_INFO("%s", entityInfo);
		}
	}
}

void
GameShutdown()
{
	TileMapFree(&State.TileMap);

	UnloadAssets(&State);

	CloseWindow();
}

internal void 
LoadAssets(GameState* gameState)
{
	gameState->AssetMgr.TileSpriteSheet = LoadTexture("Game/assets/16x16.bmp");
	gameState->AssetMgr.EntitySpriteSheet = LoadTexture("Game/assets/SpriteSheet.png");
}

internal void 
UnloadAssets(GameState* gameState)
{
	UnloadTexture(gameState->AssetMgr.TileSpriteSheet);
	UnloadTexture(gameState->AssetMgr.EntitySpriteSheet);
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
	return zpl_arena_allocator(&State.GameMemory);
}
