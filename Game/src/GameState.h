#pragma once

#include "Core.h"
#include "TileMap.h"

struct GameState
{
	zpl_stack_memory FrameStack;
	zpl_allocator FrameAllocator;
	TileMap TileMap;
};

#define DeltaTime GetFrameTime()

GameState* GetGameState();
