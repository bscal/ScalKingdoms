#include "Systems.h"

#include "GameState.h"
#include "Components.h"
#include "Sprite.h"
#include "RenderUtils.h"
#include "TileMap.h"

#include <raylib/src/raymath.h>

#define HashTile(vec) zpl_fnv32a(&vec, sizeof(Vec2i))

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

	TileMap* tilemap = &GetGameState()->TileMap;
	HashMap* entityMap = &GetGameState()->EntityMap;

	for (int i = 0; i < it->count; ++i)
	{
		if (moves[i].x || moves[i].y)
		{
			Vec2i directionVec = { (int)moves[i].x, (int)moves[i].y };

			moves[i].x = 0;
			moves[i].y = 0;

			float movespeed = 160.0f * it->delta_time;

			Vec2 pos;
			pos.x = transforms[i].Pos.x + directionVec.x * movespeed;
			pos.y = transforms[i].Pos.y + directionVec.y * movespeed;

			Vec2i tilePos;
			tilePos.x = (int)zpl_floor((pos.x + HALF_TILE_SIZE) * INVERSE_TILE_SIZE);
			tilePos.y = (int)zpl_floor((pos.y + HALF_TILE_SIZE) * INVERSE_TILE_SIZE);

			if (!IsTileInBounds(tilemap, tilePos))
				continue;

			Tile* tile = GetTile(tilemap, tilePos);
			SASSERT(tile);

			if (!tile->Flags.Get(TILE_FLAG_SOLID))
			{

				if (tilePos != transforms[i].TilePos)
				{
					zpl_u32 tileHash = HashTile(tilePos);
					ecs_entity_t* occupiedEntity = HashMapGet(entityMap, tileHash, ecs_entity_t);
					if (!occupiedEntity)
					{
						HashMapSet(entityMap, tileHash, it->entities[i], ecs_entity_t);

						zpl_u32 oldPos = HashTile(transforms[i].TilePos);
						HashMapRemove(entityMap, oldPos);
					}
					else
						continue;
				}

				transforms[i].Pos = pos;
				transforms[i].TilePos = tilePos;
			}
		}
	}
}

struct IntervalSystem
{
	float Rate; // In seconds
	float UpdateAccumulator;
	int Index;
};

void TestSystem(ecs_iter_t* it)
{
	CTransform* transforms = ecs_field(it, CTransform, 1);
	CMove* moves = ecs_field(it, CMove, 2);

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
