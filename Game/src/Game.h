#pragma once

#include "Core.h"

#include "Structures/ArrayList.h"
#include "Structures/HashMapKV.h"
#include "Structures/HashMapT.h"

struct GameState;
struct CItemStack;

struct Stockpile
{
	HashMapKV Quantities;
	ArrayList(Vec2i) Tiles;
};

struct ItemStackHolder
{
	HashMapT<u16, u16> Quantities;
	ArrayList(ecs_entity_t) ItemStacks;
};

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
