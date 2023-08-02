#pragma once

#include "Core.h"

struct CTransform
{
	Vec2 Pos;

	_FORCE_INLINE_ Vec2 ToWorld() const 
	{ 
		return
		{ 
			zpl_floor((float)Pos.x * TILE_SIZE),
			zpl_floor((float)Pos.y * TILE_SIZE)
		};
	}
};

struct CRender
{
	float Scale;
	Color Color;
	uint16_t SpriteId;
	Direction Direction;
};

struct CVelocity
{
	float x;
	float y;
	float ax;
	float ay;
};

struct CCollider
{
	Rectangle Rec;
};
