#pragma once

#include "Core.h"

#include "Structures/BitArray.h"
#include "Structures/HashSetT.h"

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
	RegionMoveData MoveData;
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


struct CItemStack
{
	u16 Type;
	u16 Quantity;
};

#ifdef COMPONENT_DECLARATION

ECS_COMPONENT_DECLARE(CTransform);
ECS_COMPONENT_DECLARE(CRender);
ECS_COMPONENT_DECLARE(CMove);

ECS_TAG_DECLARE(GameObject);

#else

extern ECS_COMPONENT_DECLARE(CTransform);
extern ECS_COMPONENT_DECLARE(CRender);
extern ECS_COMPONENT_DECLARE(CMove);

extern ECS_TAG_DECLARE(GameObject);

#endif