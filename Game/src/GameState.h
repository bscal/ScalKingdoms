#pragma once

#include "GameMode.h"

#include "Core.h"
#include "TileMap.h"
#include "Actions.h"
#include "Pathfinder.h"
#include "GUI.h"

#include "Structures/HashMap.h"

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

struct GameClient
{
	ecs_entity_t Player;
	ecs_entity_t SelectedEntity;
};

#define DeltaTime GetFrameTime()

GameState* GetGameState();

GameClient* GetClient();

zpl_allocator GetGameAllocator();
