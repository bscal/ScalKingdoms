#include "Systems.h"

#include "GameState.h"
#include "Components.h"
#include "Sprite.h"
#include "RenderUtils.h"
#include "TileMap.h"
#include "Regions.h"

#include <raylib/src/raymath.h>

void DrawEntities(ecs_iter_t* it)
{
	CTransform* transforms = ecs_field(it, CTransform, 1);
	CRender* renders = ecs_field(it, CRender, 2);

	Texture2D* texture = &GetGameState()->AssetMgr.EntitySpriteSheet;

	for (int i = 0; i < it->count; ++i)
	{
		Sprite* sprite = SpriteGet(renders[i].SpriteId);
		Rectangle dst;
		dst.x = transforms[i].Pos.x;
		dst.y = transforms[i].Pos.y;
		dst.width = renders[i].Width;
		dst.height = renders[i].Height;
		DrawSprite(texture, sprite->CastRect(), dst, sprite->Origin, renders[i].Color, false);
	}
}

void MoveOnAdd(ecs_iter_t* it)
{
	CMove* moves = ecs_field(it, CMove, 1);
	for (int i = 0; i < it->count; ++i)
	{
		moves[i] = {};
		ArrayListReserve(SAllocatorGeneral(), moves[i].MoveData.StartPath, REGION_SIZE + 2);
		ArrayListReserve(SAllocatorGeneral(), moves[i].MoveData.EndPath, REGION_SIZE + 2);
		ArrayListReserve(SAllocatorGeneral(), moves[i].MoveData.RegionPath, 16);
	}
}

void MoveOnRemove(ecs_iter_t* it)
{
	CMove* moves = ecs_field(it, CMove, 1);
	for (int i = 0; i < it->count; ++i)
	{
		ArrayListFree(SAllocatorGeneral(), moves[i].MoveData.StartPath);
		ArrayListFree(SAllocatorGeneral(), moves[i].MoveData.EndPath);
		ArrayListFree(SAllocatorGeneral(), moves[i].MoveData.RegionPath);
	}
}

void MoveSystem(ecs_iter_t* it)
{
	CTransform* transforms = ecs_field(it, CTransform, 1);
	CMove* moves = ecs_field(it, CMove, 2);

	HashMapT<Vec2i, ecs_entity_t>* entityMap = &GetGameState()->EntityMap;

	float baseMS = 8.0f * it->delta_time;

	for (int i = 0; i < it->count; ++i)
	{
#if 0
		if (zpl_array_count(moves[i].TilePath) <= 0)
			continue;
		else
		{
			Vec2i target = zpl_array_back(moves[i].TilePath);
			moves[i].Target = TileToWorldCenter(target);

			if (moves[i].Progress > 1.0f)
			{
				moves[i].Progress = 0.0f;
				moves[i].Start = moves[i].Target;
				transforms[i].Pos = moves[i].Target;
				zpl_array_pop(moves[i].TilePath);
			}
			else
			{
				transforms[i].Pos = Vector2Lerp(moves[i].Start, moves[i].Target, moves[i].Progress);
				moves[i].Progress += baseMS;
			}

			// Handle move to new tile
			Vec2i travelTilePos = WorldToTile(transforms[i].Pos);
			if (travelTilePos != transforms[i].TilePos)
			{
				HashMapTRemove(entityMap, &transforms[i].TilePos);
				HashMapTSet(entityMap, &travelTilePos, &it->entities[i]);

				transforms[i].TilePos = travelTilePos;
			}
		}
#endif
	}
}
#if 0
struct IntervalSystem
{
	float Rate; // In seconds
	float UpdateAccumulator;
	int Index;
};

void TestSystem(ecs_iter_t* it)
{
	//CTransform* transforms = ecs_field(it, CTransform, 1);
	//CMove* moves = ecs_field(it, CMove, 2);

	IntervalSystem interval = {};

	float updatesPerSecond = (float)it->count / interval.Rate;
	float updatesPerFrame = updatesPerSecond * it->delta_time;

	interval.UpdateAccumulator = fminf(interval.UpdateAccumulator + updatesPerFrame, (float)it->count);

	int updateCount = (int)interval.UpdateAccumulator;

	for (int i = 0; i < updateCount; ++i)
	{
		++interval.Index;
		if (interval.Index >= it->count)
			interval.Index = 0;

	}

	interval.UpdateAccumulator -= (float)updateCount;
}
#endif