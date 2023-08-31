#include "Game.h"

#include "GameState.h"
#include "Components.h"
#include "Pathfinder.h"
#include "Regions.h"

ecs_entity_t SpawnCreature(GameState* gamestate, u16 type, Vec2i tile)
{
	ecs_world_t* world = gamestate->World;

	ecs_entity_t entity = ecs_new_id(world);

	CTransform transform = {};
	transform.Pos = TileToWorldCenter(tile);
	transform.TilePos = tile;
	ecs_set(world, entity, CTransform, transform);
	
	ecs_add(world, entity, CMove);
	
	CRender render = {};
	render.Color = WHITE;
	render.Width = TILE_SIZE;
	render.Height = TILE_SIZE;
	ecs_set(world, entity, CRender, render);

	Vec2i pos = Vec2i{ 0, 0 };
	u32 hash = HashTile(pos);
	HashMapSet(&gamestate->EntityMap, hash, &entity);

	return entity;
}

void DestroyCreature(GameState* gamestate, ecs_entity_t entity)
{
	if (ecs_is_valid(gamestate->World, entity))
	{
		const CTransform* transform = ecs_get(gamestate->World, entity, CTransform);
		SASSERT(transform);

		u32 hash = HashTile(transform->TilePos);
		HashMapRemove(&gamestate->EntityMap, hash);

		ecs_delete(gamestate->World, entity);
	}
}

void MoveEntity(GameState* state, ecs_entity_t id, Vec2i tile)
{
	if (ecs_is_valid(state->World, id))
	{
		const CTransform* transform = ecs_get(state->World, id, CTransform);
		SASSERT(transform);

		CMove* move = ecs_get_mut(state->World, id, CMove);
		if (move)
		{
			move->Start = transform->Pos;
			move->Target = {};
			move->Progress = 0;
			FindRegionPath(&state->RegionState, &state->TileMap, transform->TilePos, tile, &move->Regions);
			RegionFindLocalPath(&state->RegionState, transform->TilePos, tile, move);
			ecs_modified(state->World, id, CMove);
		}
	}
}