#pragma once

#include "Core.h"

#include "Structures/BitArray.h"
#include "Structures/HashSet.h"

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
	HashSet Regions;
	zpl_array(Vec2i) TilePath;
	Vec2 Start;
	Vec2 Target;
	float Progress;
};

struct CBody
{
	Vec2 Velocity;
	Vec2 HitOffset;
};

struct CHealth
{
	u16 Blood;
	u16 MaxBlood;
	Flag8 Flags;
};

#ifdef COMPONENT_DECLARATION

ECS_COMPONENT_DECLARE(CTransform);
ECS_COMPONENT_DECLARE(CRender);
ECS_COMPONENT_DECLARE(CMove);

#else

extern ECS_COMPONENT_DECLARE(CTransform);
extern ECS_COMPONENT_DECLARE(CRender);
extern ECS_COMPONENT_DECLARE(CMove);

#endif