#pragma once

#include "Core.h"
#include "TileMap.h"
#include "Regions.h"
#include "Actions.h"
#include "Pathfinder.h"
#include "GUI.h"
#include "TextureAtlas.h"
#include "GameTypes.h"
#include "Components.h"
#include "Entity.h"

#include "Structures/SList.h"
#include "Structures/ArrayList.h"
#include "Structures/HashMap.h"

#include "Lib/Arena.h"
#include "Lib/LinearArena.h"
#include "Lib/GeneralAllocator.h"
#include "Lib/Jobs.h"

#include <luajit/src/lua.hpp>

enum GUIScale
{
	SCALE_050,
	SCALE_075,
	SCALE_NORMAL,
	SCALE_150,
	SCALE_200,

	SCALE_MAX
};
constant_var Vec2 GUIScaleResolutions[SCALE_MAX] = { { 0.5f, 0.5f }, { 0.75f, 0.75f }, { 1.0f, 1.0f }, { 1.5f, 1.5f }, { 2.0f, 2.0f } };

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
	Arena GameArena;
	GeneralPurposeAllocator GeneralPurposeMemory;

	RenderTexture2D ScreenTexture;
	RenderTexture2D GUITexture;

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
	Arena TransientArena;
};

struct GameClient
{
	Vec2i GameResolution;
	RectI TileViewUpdateRect;
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

global_var struct GameState State;
global_var struct TransientGameState TransientState;
global_var struct GameClient Client;

#define DeltaTime GetFrameTime()

#define GetGameState() (&State)

_FORCE_INLINE_ static Vec2i
ScreenToTile()
{
	Vec2 res = { GAME_WIDTH / (float)GetScreenWidth() , GAME_HEIGHT / (float)GetScreenHeight() };
	Vec2 scaledMouse = GetMousePosition() * Vec2 { res.x, res.y };
	Vec2 worldPos = GetScreenToWorld2D(scaledMouse, GetGameState()->Camera);
	Vec2 worldTilePos = worldPos * Vec2{ INVERSE_TILE_SIZE, INVERSE_TILE_SIZE };
	return Vec2ToVec2i(worldTilePos);
}

void ItemStackHolderAdd(ItemStackHolder* holder, CItemStack* stack);
void ItemStackHolderRemove(ItemStackHolder* holder, CItemStack* stack);
int ItemStackHolderFind(ItemStackHolder* holder, u16 itemId);
void ItemStackHolderFill(ItemStackHolder* holder, u16 itemId, ArrayList(ecs_entity_t) inItemStacks);

ecs_entity_t SpawnCreature(GameState* gamestate, u16 type, Vec2i tile);

void DestroyCreature(GameState* gamestate, ecs_entity_t entity);

void MoveEntity(GameState* state, ecs_entity_t id, Vec2i tile);

_FORCE_INLINE_ Vec2i WorldToTile(Vec2 pos)
{
	Vec2i res =
	{
		(int)floorf(pos.x / TILE_SIZE),
		(int)floorf(pos.y / TILE_SIZE)
	};
	return res;
}

_FORCE_INLINE_ Vec2 TileToWorld(Vec2i tile)
{
	Vec2 res =
	{
		(float)tile.x * TILE_SIZE,
		(float)tile.y * TILE_SIZE
	};
	return res;
}

_FORCE_INLINE_ Vec2 TileToWorldCenter(Vec2i tile)
{
	Vec2 res =
	{
		(float)tile.x * TILE_SIZE + HALF_TILE_SIZE,
		(float)tile.y * TILE_SIZE + HALF_TILE_SIZE
	};
	return res;
}
