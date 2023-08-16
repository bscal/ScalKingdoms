#pragma once

#include "Core.h"
#include "TileMap.h"
#include "Actions.h"
#include "Pathfinder.h"

#include "Structures/HashMap.h"

struct Pathfinder;

struct AssetMgr
{
	Texture2D TileSpriteSheet;
	Texture2D EntitySpriteSheet;
	Texture2D GUISpriteSheet;
};

struct GameState
{
	zpl_arena GameMemory;

	AssetMgr AssetMgr;

	TileMap TileMap;

	Camera2D Camera;

	zpl_random Random;
	ecs_world_t* World;

	HashMap EntityMap;

	ActionMgr ActionMgr;

	Pathfinder Pathfinder;
};

struct GameClient
{
	ecs_entity_t Player;
	ecs_entity_t SelectedEntity;
};

#define DeltaTime GetFrameTime()

GameState* GetGameState();

GameClient* GetClient();

zpl_allocator
GetGameAllocator();
