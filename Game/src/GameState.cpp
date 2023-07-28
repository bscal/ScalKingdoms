#include "Core.h"

#include "TileMap.cpp"

struct GameState
{
	TileMap TileMap;
};

static struct GameState GameState;

void GameRun();
void GameUpdate();
void GameShutdown();

void 
GameInitialize()
{
	InitWindow(WIDTH, HEIGHT, TITLE);

	TileMapInit(&GameState.TileMap);

	GameRun();
}

void
GameRun()
{
	while (!WindowShouldClose())
	{
		GameUpdate();

		BeginDrawing();
		ClearBackground(BLACK);
		EndDrawing();
	}

	GameShutdown();
}

void GameUpdate()
{

}

void
GameShutdown()
{
	CloseWindow();
}