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
		//Vec2 worldPos = transforms[i].ToWorld();
		Rectangle dst;
		dst.x = transforms[i].Pos.x;
		dst.y = transforms[i].Pos.y;
		dst.width = TILE_SIZE * renders[i].Scale;
		dst.height = TILE_SIZE * renders[i].Scale;
		DrawSprite(SpriteGetTexture(), sprite->CastRect(), dst, sprite->Origin, renders[i].Color, false);
	}
}

internal _FORCE_INLINE_ bool 
HandleCollision(Rectangle rec1, Rectangle rec2, Vec2 lastPos, Vec2* newPos)
{
	if (CheckCollisionRecs(rec1, rec2))
	{
		if (rec1.x <= rec2.x)
			newPos->x = lastPos.x;
		
		if (rec1.y <= rec2.y)
			newPos->y = lastPos.y;

		return true;
	}
	return false;
}

void MoveSystem(ecs_iter_t* it)
{
	CTransform* transforms = ecs_field(it, CTransform, 1);
	CVelocity* velocitys = ecs_field(it, CVelocity, 2);
	CCollider* colliders = ecs_field(it, CCollider, 3);

	TileMap* tilemap = &GetGameState()->TileMap;

	float drag = .7f;

	for (int i = 0; i < it->count; ++i)
	{
		CVelocity* v = &velocitys[i];
		v->x += v->ax;
		v->y += v->ay;

		Vec2 newPos =
		{
			transforms[i].Pos.x += v->x,
			transforms[i].Pos.y += v->y
		};

		v->x *= drag;
		if (v->x <= .1 && v->x >= -.1)
			v->x = 0;
		v->y *= drag;
		if (v->y <= .1 && v->y >= -.1)
			v->y = 0;
	}

	for (int i = 0; i < it->count; ++i)
	{
		Vec2i tilePos;
		tilePos.x = (int)zpl_floor((transforms[i].Pos.x + 8) * TILE_SIZE);
		tilePos.y = (int)zpl_floor((transforms[i].Pos.y + 8) * TILE_SIZE);

		Tile* tile = GetTile(tilemap, tilePos);
	}
}
