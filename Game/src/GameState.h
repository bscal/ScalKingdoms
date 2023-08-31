#pragma once

#include "Game.h"

#include "Core.h"
#include "TileMap.h"
#include "Regions.h"
#include "Actions.h"
#include "Pathfinder.h"
#include "GUI.h"

#include "Structures/SList.h"
#include "Structures/ArrayList.h"
#include "Structures/HashMap.h"

#include <luajit/src/lua.hpp>

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
	RegionState RegionState;
};

struct GameClient
{
	ecs_entity_t Player;
	ecs_entity_t SelectedEntity;

	double FrameTime;

	// Extra debug values
	zpl_array(Vec2i) PathfinderVisited;
	zpl_array(Vec2i) PathfinderPath;
	ArrayList(Vec2i) DebugRegionsPath;

	bool IsDebugMode;

	union
	{
		bool DebugOptions[1];
		struct
		{
			bool DebugShowRegionPaths;
		};
	};
};

extern struct GameClient Client;

#define DeltaTime GetFrameTime()

GameState* GetGameState();

zpl_allocator GetGameAllocator();
