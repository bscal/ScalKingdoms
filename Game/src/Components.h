#pragma once

#include "Core.h"

struct CTransform
{
	Vec2 Pos;
	Vec2i TilePos;
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
	Vec2i Target;
	int8_t x; // In tiles
	int8_t y; // In tiles
};

struct CBody
{
	Vec2 Velocity;
	Vec2 HitOffset;
};
