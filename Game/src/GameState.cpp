#include "GameState.h"
#include "GameEntry.h"

#include "Tile.h"
#include "Sprite.h"
#include "Components.h"
#include "Systems.h"

#include <raylib/src/raymath.h>

global_var struct GameState State;
global_var struct GameClient Client;

internal void GameRun();
internal void GameUpdate();
internal void GameShutdown();
internal void InputUpdate();
internal Vec2i ScreenToTile(Vec2 pos);

ECS_COMPONENT_DECLARE(CTransform);
ECS_COMPONENT_DECLARE(CMove);

int 
GameInitialize()
{
	zpl_stack_memory_init(&State.FrameStack, zpl_heap_allocator(), Megabytes(4));
	State.FrameAllocator = zpl_stack_allocator(&State.FrameStack);

	InitWindow(WIDTH, HEIGHT, TITLE);
	SetTraceLogLevel(LOG_ALL);
	SetTargetFPS(60);

	State.Camera.zoom = 1.0f;
	State.Camera.offset = { (float)GetScreenWidth() / 2, (float)GetScreenHeight() / 2 };

	State.TileSpriteSheet = LoadTexture("Game/assets/16x16.bmp");
	State.EntitySpriteSheet = LoadTexture("Game/assets/SpriteSheet.png");

	State.EntityMap.Initialize(sizeof(ecs_entity_t), 64, DefaultAllocFunc, DefaultFreeFunc);

	zpl_random_init(&State.Random);

	SpriteMgrInitialize(State.EntitySpriteSheet);
	Sprite* sprite = SpriteGet(Sprites::PLAYER);

	TileMgrInitialize(&State.TileSpriteSheet);

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

	GameRun();

	GameShutdown();

	return 0;
}

void
GameRun()
{
	while (!WindowShouldClose())
	{
		zpl_free_all(State.FrameAllocator);

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
		DrawRectangleLines((int)target.x, (int)target.y, 16, 16, MAGENTA);

		EndMode2D();

		EndDrawing();
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

	if (IsKeyPressed(KEY_W))
	{
		movement.x += VEC2_UP.x * DeltaTime * 160;
		movement.y += VEC2_UP.y * DeltaTime * 160;
		moveY = -1;
	}
	else if (IsKeyPressed(KEY_S))
	{
		movement.x += VEC2_DOWN.x * DeltaTime * 160;
		movement.y += VEC2_DOWN.y * DeltaTime * 160;
		moveY = 1;
	}

	if (IsKeyPressed(KEY_A))
	{
		movement.x += VEC2_LEFT.x * DeltaTime * 160;
		movement.y += VEC2_LEFT.y * DeltaTime * 160;
		moveX = -1;
	}
	else if (IsKeyPressed(KEY_D))
	{
		movement.x += VEC2_RIGHT.x * DeltaTime * 160;
		movement.y += VEC2_RIGHT.y * DeltaTime * 160;
		moveX = 1;
	}

	ecs_set_ex(State.World, Client.Player, CMove, { moveX, moveY });

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

		Tile t = {};
		t.TileId = Tiles::WOOD_DOOR;
		t.Flags.Set(TILE_FLAG_COLLISION, true);
		SetTile(&State.TileMap, tile, &t, 0);
	}

	if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
	{
		Vec2i tile = ScreenToTile(GetMousePosition());

		u32 hash = zpl_fnv32a(&tile, sizeof(Vec2i));
		ecs_entity_t* entity = HashMapGet(State.EntityMap, hash, ecs_entity_t);
		if (entity)
		{
			const char* entityInfo = ecs_entity_str(State.World, *entity);
			SLOG_INFO("%s", entityInfo);
		}
	}
}

void
GameShutdown()
{
	TileMapFree(&State.TileMap);

	UnloadTexture(State.TileSpriteSheet);
	UnloadTexture(State.EntitySpriteSheet);

	CloseWindow();
}

internal Vec2i 
ScreenToTile(Vec2 pos)
{
	Vec2 worldPos = GetScreenToWorld2D(pos, State.Camera);
	worldPos = Vector2Multiply(worldPos, { INVERSE_TILE_SIZE, INVERSE_TILE_SIZE });
	return Vec2ToVec2i(worldPos);
}

GameState* GetGameState()
{
	return &State;
}

GameClient* GetClient()
{
	return &Client;
}
