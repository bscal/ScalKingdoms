#pragma once

#include "Core.h"
#include "TileMap.h"

struct GameState
{
	zpl_stack_memory FrameStack;
	zpl_allocator FrameAllocator;
	TileMap TileMap;

	Camera2D Camera;
	Texture2D TileSpriteSheet;
	Texture2D EntitySpriteSheet;
};

struct GameClient
{
	ecs_entity_t Player;
};

#define DeltaTime GetFrameTime()

zpl_random* GetRandom();

GameState* GetGameState();

GameClient* GetClient();
