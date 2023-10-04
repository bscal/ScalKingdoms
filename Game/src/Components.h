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
	bool IsCompleted;
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

inline ECS_COMPONENT_DECLARE(CTransform);
inline ECS_COMPONENT_DECLARE(CRender);
inline ECS_COMPONENT_DECLARE(CMove);
inline ECS_TAG_DECLARE(GameObject);

