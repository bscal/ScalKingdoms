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

#include "Lib/Arena.h"
#include "Lib/LinearArena.h"
#include "Lib/GeneralAllocator.h"
#include "Lib/Jobs.h"

#include <luajit/src/lua.hpp>

struct AssetMgr
{
	Texture2D TileSpriteSheet;
	Texture2D EntitySpriteSheet;
	Texture2D ShapesTexture;

	Font MainFont;
	SpriteAtlas UIAtlas;
};

struct GameState
{
	GeneralPurposeAllocator GameMemory;
	LinearArena FrameMemory;

	RenderTexture2D ScreenTexture;

	AssetMgr AssetMgr;

	GUIState GUIState;

	TileMap TileMap;

	Camera2D Camera;

	ecs_world_t* World;

	HashMapT<Vec2i, ecs_entity_t> EntityMap;

	ActionMgr ActionMgr;

	Pathfinder Pathfinder;
	RegionPathfinder RegionPathfinder;

	bool IsGamePaused;
};

struct TransientGameState
{
	Arena PermanentMemory;
};

struct GameClient
{
	Vec2i GameResolution;

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
	bool IsErrorWindowOpen;
};

global_var struct TransientGameState TransientState;
global_var struct GameClient Client;

#define DeltaTime GetFrameTime()

GameState* GetGameState();
