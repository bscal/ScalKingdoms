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

#define DeltaTime GetFrameTime()

GameState* GetGameState();
