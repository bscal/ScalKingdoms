#include "GameState.h"

ecs_entity_t SpawnCreature(GameState* gamestate, u16 type, Vec2i tile)
{
	ecs_world_t* world = gamestate->World;

	ecs_entity_t entity = ecs_new_id(world);
	// TODO handle name
	//ecs_set_name(world, entity, "PlayerName");

	CTransform transform = {};
	transform.Pos = TileToWorldCenter(tile);
	transform.TilePos = tile;
	ecs_set_ex(world, entity, CTransform, transform);
	
	CMove m = {};
	ecs_set_ex(world, entity, CMove, m);
	
	CRender render = {};
	render.Color = WHITE;
	render.Width = TILE_SIZE;
	render.Height = TILE_SIZE;
	ecs_set_ex(world, entity, CRender, render);

	Vec2i pos = Vec2i{ 0, 0 };
	HashMapTSet(&gamestate->EntityMap, &pos, &entity);

	return entity;
}

void DestroyCreature(GameState* gamestate, ecs_entity_t entity)
{
	if (ecs_is_valid(gamestate->World, entity))
	{
		const CTransform* transform = ecs_get(gamestate->World, entity, CTransform);
		SAssert(transform);

		HashMapTRemove(&gamestate->EntityMap, &transform->TilePos);

		ecs_delete(gamestate->World, entity);
	}
}

void MoveEntity(GameState* state, ecs_entity_t id, Vec2i tile)
{
	if (ecs_is_valid(state->World, id))
	{
		const CTransform* transform = ecs_get(state->World, id, CTransform);
		SAssert(transform);

		CMove* move = ecs_get_mut(state->World, id, CMove);
		if (move)
		{
			move->Start = transform->Pos;
			move->Target = {};
			move->Progress = 0;
			move->IsCompleted = false;
			{
				Timer();
				PathfindRegion(transform->TilePos, tile, &move->MoveData);
			}
			ecs_modified(state->World, id, CMove);
		}
	}
}