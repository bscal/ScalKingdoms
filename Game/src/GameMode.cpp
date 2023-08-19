#include "GameMode.h"

#include "GameState.h"
#include "Components.h"
#include "Pathfinder.h"

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
			move->Length = (i16)PathFindArrayFill(move->Path, &state->Pathfinder, &state->TileMap, transform->TilePos, tile);
			move->Index = move->Length;
			ecs_modified(state->World, id, CMove);
		}
	}
}