#pragma once

#include "Core.h"
#include "TileMap.h"
#include "Actions.h"

#include "Structures/HashMap.h"

struct AssetMgr
{
	Texture2D TileSpriteSheet;
	Texture2D EntitySpriteSheet;
	Texture2D GUISpriteSheet;
};

struct GameState
{
	AssetMgr AssetMgr;

	zpl_stack_memory FrameStack;
	zpl_allocator FrameAllocator;
	TileMap TileMap;

	Camera2D Camera;

	zpl_random Random;
	ecs_world_t* World;

	HashMap EntityMap;

	ActionMgr ActionMgr;
};

struct GameClient
{
	ecs_entity_t Player;
	ecs_entity_t SelectedEntity;
};

#define DeltaTime GetFrameTime()

GameState* GetGameState();

GameClient* GetClient();
