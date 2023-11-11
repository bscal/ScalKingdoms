#pragma once

#include "Core.h"

#include "Structures/StaticArray.h"
#include "Structures/BitArray.h"
#include "Structures/SparseSet.h"

struct Entity
{
	u32 Id;
	u16 Gen;
	u8 Type;
};

constant_var int MAX_COMPONENTS = 64;
constant_var int MAX_ENTITIES = 4096;

global_var int GlobalComponentIdCounter = 0;

template<typename C>
struct Component
{
	static int ID = GlobalComponentIdCounter++;
};

#define COMPONENT_TYPE(type) struct type : Component<type>

COMPONENT_TYPE(Velocity)
{
	int x;
};

struct WorldEntity
{
	Vec2 Pos;
	Vec2i TilePos;
	float Rotation;
	float Scale;
	Color Color;
	u16 SpriteId;
	Flag8 Flags;
};

struct WorldProp
{
	Vec2 Pos;
};

struct EntityMgr
{
	Buffer<WorldEntity, MAX_ENTITIES> WorldEntities;

	int NextEntity;
	StaticArray<Entity, MAX_ENTITIES> EntityPool;

	StaticArray<SparseSet<void*>, MAX_COMPONENTS> Components;
};

int EntitiesInitialize();
