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
	Vec2 Start;
	Vec2 Target;
	float Progress;
	i16 Length;
	i16 Index;
	Vec2i Path[MAX_PATHFIND_LENGTH];
};

struct CBody
{
	Vec2 Velocity;
	Vec2 HitOffset;
};

#ifdef COMPONENT_DECLARATION

ECS_COMPONENT_DECLARE(CTransform);
ECS_COMPONENT_DECLARE(CMove);

#else

extern ECS_COMPONENT_DECLARE(CTransform);
extern ECS_COMPONENT_DECLARE(CMove);

#endif