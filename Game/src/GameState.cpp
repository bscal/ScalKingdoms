#include "GameState.h"
#include "GameEntry.h"

#include <raylib/src/raymath.h>

#include "Structures/HashMap.h"

global_var struct GameState State;

internal void GameRun();
internal void GameUpdate();
internal void GameShutdown();
internal void InputUpdate();

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

	TileMapInit(&State, &State.TileMap);

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
		screenRect.width = (float)GetScreenWidth();
		screenRect.height = (float)GetScreenHeight();

		BeginDrawing();
		ClearBackground(BLACK);
		BeginMode2D(State.Camera);

		TileMapDraw(&State.TileMap, screenRect);

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

	State.Camera.target = Vector2Add(State.Camera.target, movement);
	State.CameraPosition = State.Camera.target;

	float mouseWheel = GetMouseWheelMove();
	if (mouseWheel != 0)
	{
		constexpr float ZOOM_MAX = 5.0f;
		constexpr float ZOOM_MIN = 1.0f;
		constexpr float ZOOM_SPD = .20f;

		float newZoom = State.Camera.zoom + ZOOM_SPD * mouseWheel;

		if (newZoom > ZOOM_MAX)
			newZoom = ZOOM_MAX;
		else if (newZoom < ZOOM_MIN)
			newZoom = ZOOM_MIN;

		State.Camera.zoom = newZoom;
	}
}

void
GameShutdown()
{
	TileMapFree(&State.TileMap);

	CloseWindow();
}

GameState* GetGameState()
{
	return &State;
}
