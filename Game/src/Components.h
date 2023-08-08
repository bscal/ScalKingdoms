#pragma once

#include "Core.h"

struct CTransform
{
	Vec2 Pos;

	_FORCE_INLINE_ Vec2i ToWorld() const 
	{ 
		return
		{ 
			(int)zpl_floor((float)Pos.x * INVERSE_TILE_SIZE),
			(int)zpl_floor((float)Pos.y * INVERSE_TILE_SIZE)
		};
	}
};

struct CRender
{
	Color Color;
	uint16_t SpriteId;
	uint8_t Width;
	uint8_t Height;
};

struct CMove
{
	int8_t x; // In tiles
	int8_t y; // In tiles
};

struct CBody
{
	Vec2 Velocity;
	Vec2 HitOffset;
};
