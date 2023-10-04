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

void MoveSystem(ecs_iter_t* it)
{
	CTransform* transforms = ecs_field(it, CTransform, 1);
	CMove* moves = ecs_field(it, CMove, 2);

	HashMapT<Vec2i, ecs_entity_t>* entityMap = &GetGameState()->EntityMap;

	float baseMS = 8.0f * it->delta_time;
#if 1
	for (int i = 0; i < it->count; ++i)
	{
		if (moves[i].IsCompleted)
			continue;

		u8 pathType;
		Vec2i target;
		// > 1 because index[0] or (last position in path) == start of region path. We will pause on that tile.
		if (moves[i].MoveData.StartPathLength > 1)
		{
			// Gets last index since paths are from back to front order
			target = moves[i].MoveData.StartPath[moves[i].MoveData.StartPathLength - 1];
			pathType = 0;
		}
		else if (moves[i].MoveData.PathLength > 0)
		{
			RegionPath pathData = moves[i].MoveData.Path[moves[i].MoveData.PathLength - 1];
			Region* region = GetRegion(pathData.RegionCoord);
			SAssert(region);
			Vec2i* path = region->Paths[(size_t)pathData.Direction];
			SAssert(path);
			int pathLength = ArrayListCount(path) - 1;
			target = path[pathLength - moves[i].MoveData.PathProgress];
			pathType = 1;
		}
		// > 1 because we - 2 on endpath below. This skips the 1st element so we don't pause on it
		else if (moves[i].MoveData.EndPathLength > 1)
		{
			target = moves[i].MoveData.EndPath[moves[i].MoveData.EndPathLength - 2];
			pathType = 2;
		}
		else
		{
			moves[i].IsCompleted = true;
			continue;
		}

		moves[i].Target = Vec2iToVec2(target) * Vec2 { TILE_SIZE, TILE_SIZE } + Vec2{ HALF_TILE_SIZE, HALF_TILE_SIZE };
		transforms[i].Pos = Vector2Lerp(moves[i].Start, moves[i].Target, moves[i].Progress);
		moves[i].Progress += baseMS;

		if (moves[i].Progress > 1.0f)
		{
			moves[i].Progress = 0.0f;
			moves[i].Start = moves[i].Target;
			transforms[i].Pos = moves[i].Target;

			if (pathType == 0)
				--moves[i].MoveData.StartPathLength;
			else if (pathType == 1)
			{
				RegionPath pathData = moves[i].MoveData.Path[moves[i].MoveData.PathLength - 1];
				Region* region = GetRegion(pathData.RegionCoord);
				Vec2i* path = region->Paths[(size_t)pathData.Direction];
				int pathLength = ArrayListCount(path);
				++moves[i].MoveData.PathProgress;
				if (moves[i].MoveData.PathProgress == ArrayListCount(path))
				{
					moves[i].MoveData.PathProgress = 0;
					--moves[i].MoveData.PathLength;
				}
			}
			else if (pathType == 2)
				--moves[i].MoveData.EndPathLength;
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