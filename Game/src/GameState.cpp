#include "GameState.h"
#include "GameEntry.h"

#include "Sprite.h"
#include "Components.h"
#include "Systems.h"

#include <raylib/src/raymath.h>


global_var struct GameState State;
global_var struct ecs_world_t* World;

struct GameClient
{
	ecs_entity_t Player;
};
global_var GameClient Client;

internal void GameRun();
internal void GameUpdate();
internal void GameShutdown();
internal void InputUpdate();
internal Vec2i ScreenToTile(Vec2 pos);

ECS_COMPONENT_DECLARE(CTransform);
ECS_COMPONENT_DECLARE(CVelocity);

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

	SpriteMgrInitialize(State.EntitySpriteSheet);
	Sprite* sprite = SpriteGet(Sprites::PLAYER);

	TileMapInit(&State, &State.TileMap);

	// Entities
	World = ecs_init();

	ECS_COMPONENT_DEFINE(World, CTransform);
	ECS_COMPONENT(World, CRender);
	ECS_COMPONENT_DEFINE(World, CVelocity);
	ECS_COMPONENT(World, CCollider);

	ECS_SYSTEM(World, DrawEntities, EcsOnUpdate, CTransform, CRender);
	ECS_SYSTEM(World, MoveSystem, EcsOnUpdate, CTransform, CVelocity, CCollider);

	Client.Player = ecs_new_id(World);
	ecs_add(World, Client.Player, CTransform);
	ecs_add(World, Client.Player, CRender);
	ecs_add(World, Client.Player, CVelocity);
	ecs_add(World, Client.Player, CCollider);
	
	ecs_set_ex(World, Client.Player, CTransform, {});
	CRender render = {};
	render.Scale = 1;
	render.Color = WHITE;
	ecs_set(World, Client.Player, CRender, render);

	ecs_set_ex(World, Client.Player, CVelocity, {});

	CCollider c = { { 1, 1, 15.f, 15.f } };
	ecs_set(World, Client.Player, CCollider, c);

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

		ecs_progress(World, DeltaTime);

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

	CVelocity velocity = *ecs_get(World, Client.Player, CVelocity);
	velocity.ax = movement.x;
	velocity.ay = movement.y;
	ecs_set(World, Client.Player, CVelocity, velocity);

	//const CTransform* transform = ecs_get(World, Client.Player, CTransform);
	//State.Camera.target = Vector2Add(State.Camera.target, transform->Pos);

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
		t.TileId = 8;
		t.Flags.Set(TILE_FLAG_COLLISION, true);
		SetTile(&State.TileMap, tile, &t);
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
