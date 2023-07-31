#pragma once

#include "Core.h"
#include "TileMap.h"

struct GameState
{
	zpl_stack_memory FrameStack;
	zpl_allocator FrameAllocator;
	TileMap TileMap;

	Camera2D Camera;
	Vector2 CameraPosition;
	Texture2D TileSpriteSheet;
};

#define DeltaTime GetFrameTime()

GameState* GetGameState();
