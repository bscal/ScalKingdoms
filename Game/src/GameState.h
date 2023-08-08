#pragma once

#include "Core.h"
#include "TileMap.h"

#include "Structures/HashMap.h"

struct GameState
{
	zpl_stack_memory FrameStack;
	zpl_allocator FrameAllocator;
	TileMap TileMap;

	Camera2D Camera;
	Texture2D TileSpriteSheet;
	Texture2D EntitySpriteSheet;

	zpl_random Random;
	ecs_world_t* World;

	HashMap EntityMap;
};

struct GameClient
{
	ecs_entity_t Player;
};

#define DeltaTime GetFrameTime()

GameState* GetGameState();

GameClient* GetClient();
