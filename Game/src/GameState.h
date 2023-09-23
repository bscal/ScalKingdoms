#pragma once

#include "Game.h"

#include "Core.h"
#include "TileMap.h"
#include "Regions.h"
#include "Actions.h"
#include "Pathfinder.h"
#include "GUI.h"
#include "TextureAtlas.h"

#include "Structures/SList.h"
#include "Structures/ArrayList.h"
#include "Structures/HashMap.h"
#include "Lib/MemoryArena.h"

#include <luajit/src/lua.hpp>

struct AssetMgr
{
	Texture2D TileSpriteSheet;
	Texture2D EntitySpriteSheet;

	Font MainFont;
	SpriteAtlas UIAtlas;
};

struct GameState
{
	MemArena GameMemory;
	zpl_arena FrameMemory;
	zpl_jobs_system Jobs;

	RenderTexture2D ScreenTexture;

	AssetMgr AssetMgr;

	GUIState GUIState;

	TileMap TileMap;

	Camera2D Camera;

	zpl_random Random;
	ecs_world_t* World;

	HashMapT<Vec2i, ecs_entity_t> EntityMap;

	ActionMgr ActionMgr;

	Pathfinder Pathfinder;
	RegionPathfinder RegionPathfinder;
	RegionState RegionState;
};

struct GameClient
{
	ecs_entity_t Player;
	ecs_entity_t SelectedEntity;

	RegionMoveData MoveData;

	double UpdateTime; // In seconds

	// Extra debug values
	ArrayList(Vec2i) PathfinderVisited;
	ArrayList(Vec2i) PathfinderPath;
	ArrayList(Vec2i) DebugPathfinder;

	bool IsConsoleOpen;
	bool IsDebugWindowOpen;
	bool IsDebugMode;
};

extern struct GameClient Client;

#define DeltaTime GetFrameTime()

GameState* GetGameState();

zpl_allocator GetGameAllocator();
zpl_allocator GetFrameAllocator();
