#pragma once

#include "Core.h"

struct CTransform
{
	Vec2i Pos;
};

struct CRender
{
	uint16_t SpriteId;
	Color Color;
	Direction Direction;
};
