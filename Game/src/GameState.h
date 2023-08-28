#pragma once

#include "GameMode.h"

#include "Core.h"
#include "TileMap.h"
#include "Actions.h"
#include "Pathfinder.h"
#include "GUI.h"

#include "Structures/SList.h"
#include "Structures/HashMap.h"

#include <luajit/src/lua.hpp>

//extern ECS_COMPONENT_DECLARE(CTransform);
//extern ECS_COMPONENT_DECLARE(CMove);

struct Pathfinder;

struct AssetMgr
{
	Texture2D TileSpriteSheet;
	Texture2D EntitySpriteSheet;
	Texture2D GUISpriteSheet;

	Font MainFont;
};

struct GameState
{
	zpl_arena GameMemory;

	zpl_jobs_system Jobs;

	RenderTexture2D ScreenTexture;

	AssetMgr AssetMgr;

	GUIState GUIState;

	TileMap TileMap;

	Camera2D Camera;

	zpl_random Random;
	ecs_world_t* World;

	HashMap EntityMap;

	ActionMgr ActionMgr;

	Pathfinder Pathfinder;
};

enum TileMapDebugFlags
{
	TILE_MAP_DEBUG_FLAG_PATHFINDING = Bit(0)
};

struct GameClient
{
	ecs_entity_t Player;
	ecs_entity_t SelectedEntity;

	Flag32 TileMapDebugFlag;

	// Extra debug values
	zpl_array(Vec2i) PathfinderVisited;
	zpl_array(Vec2i) PathfinderPath;

	bool IsDebugMode;
};

extern struct GameClient Client;

#define DeltaTime GetFrameTime()

GameState* GetGameState();

zpl_allocator GetGameAllocator();
