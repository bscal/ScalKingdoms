#include "GameState.h"
#include "GameEntry.h"

struct GameState State;

void GameRun();
void GameUpdate();
void GameShutdown();

int 
GameInitialize()
{
	zpl_stack_memory_init(&State.FrameStack, zpl_heap_allocator(), Megabytes(4));
	State.FrameAllocator = zpl_stack_allocator(&State.FrameStack);

	InitWindow(WIDTH, HEIGHT, TITLE);
	SetTraceLogLevel(LOG_ALL);
	SetTargetFPS(60);

	TileMapInit(&State, &State.TileMap);

	GameRun();

	return 0;
}

void
GameRun()
{
	while (!WindowShouldClose())
	{
		zpl_free_all(State.FrameAllocator);

		GameUpdate();

		BeginDrawing();
		ClearBackground(BLACK);
		EndDrawing();
	}

	GameShutdown();
}

void GameUpdate()
{
	TileMapUpdate(&State, &State.TileMap);
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
