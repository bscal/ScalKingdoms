#pragma once

#include "Core.h"

struct GameState;

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
