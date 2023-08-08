#include "Systems.h"

#include "GameState.h"
#include "Components.h"
#include "Sprite.h"
#include "RenderUtils.h"
#include "TileMap.h"

#include <raylib/src/raymath.h>

void DrawEntities(ecs_iter_t* it)
{
	CTransform* transforms = ecs_field(it, CTransform, 1);
	CRender* renders = ecs_field(it, CRender, 2);

	for (int i = 0; i < it->count; ++i)
	{
		Sprite* sprite = SpriteGet(renders[i].SpriteId);
		Rectangle dst;
		dst.x = transforms[i].Pos.x;
		dst.y = transforms[i].Pos.y;
		dst.width = renders[i].Width;
		dst.height = renders[i].Height;
		DrawSprite(SpriteGetTexture(), sprite->CastRect(), dst, sprite->Origin, renders[i].Color, false);
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
			Vec2i playerPos = transforms->ToWorld();
			Vec2i tilePos = playerPos + directionVec;

			if (!IsTileInBounds(tilemap, tilePos))
				continue;

			Tile* tile = GetTile(tilemap, tilePos);
			SASSERT(tile);

			if (!tile->Flags.Get(TILE_FLAG_SOLID))
			{
				zpl_u32 tileHash = zpl_fnv32a(&tilePos, sizeof(Vec2i));
				ecs_entity_t* occupiedEntity = HashMapGet((*entityMap), tileHash, ecs_entity_t);
				if (!occupiedEntity)
				{
					ecs_entity_t entity = it->entities[i];
					entityMap->Put(tileHash, &entity);

					zpl_u32 plyHash = zpl_fnv32a(&playerPos, sizeof(Vec2i));
					entityMap->Remove(plyHash);

					transforms[i].Pos.x += directionVec.x * TILE_SIZE;
					transforms[i].Pos.y += directionVec.y * TILE_SIZE;
				}
			}
		}
	}
}
