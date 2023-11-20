#pragma once

#include "Core.h"
#include "Memory.h"

#include "Structures/StaticArray.h"
#include "Structures/BitArray.h"
#include "Structures/SparseSet.h"
#include "Structures/ArrayList.h"

struct Entity
{
	u32 Id;
	u16 Gen;
	u8 Type;
	bool IsAlive;
};

#define ENTITY_ALLOCATOR SAllocatorGeneral()

constant_var u8 ENTITY_TYPE_WORLDENTITY = 0;

constant_var int MAX_COMPONENTS = 64;
constant_var int MAX_ENTITIES = 1024 * 4;

global_var int GlobalComponentIdCounter = 0;

template<typename C>
struct Component
{
	const static int Id;
};

template<typename C>
const int Component<C>::Id = GlobalComponentIdCounter++;

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

struct EntityMgr
{
	StaticArray<WorldEntity, MAX_ENTITIES> WorldEntities;

	StaticArray<Entity, MAX_ENTITIES> Entities;

	u32 AliveEntities;
	u32 NextEntity;
	Buffer<u32, MAX_ENTITIES> OpenIds;

	StaticArray<SparseArray, MAX_COMPONENTS> Components;
	StaticArray<SArray, MAX_COMPONENTS> ComponentsValues;
};

#define RegisterComponent(type, allocator)	\
	do { \
		SAssert(type::Id < MAX_COMPONENTS);	\
		int componentId = type::Id;	\
		EntityManager.Components.Data[componentId].Initialize(allocator, MAX_ENTITIES);	\
		EntityManager.ComponentsValues.Data[componentId] = ArrayCreate(allocator, 8, sizeof(type));	\
	} while (0)

global_var EntityMgr EntityManager;

inline int EntitiesInitialize()
{
	RegisterComponent(Velocity, ENTITY_ALLOCATOR);
}

_FORCE_INLINE_ void* GetEntityData(Entity entity)
{
	switch (entity.Type)
	{
	case (ENTITY_TYPE_WORLDENTITY):
	{
		SAssert(entity.Id < MAX_ENTITIES);
		return EntityManager.WorldEntities.At(entity.Id);
	} 
	}

	SError("Trying to get entity data from invalid entity type!");
	return nullptr;
}

inline Entity MakeEntity(u8 entityType)
{
	Entity res;
	if (EntityManager.OpenIds.Count > 0)
	{
		res = EntityManager.Entities.AtCopy(*EntityManager.OpenIds.Last());
		EntityManager.OpenIds.Pop();
	}
	else
	{
		res.Id = EntityManager.NextEntity;
		res.Gen = 0;
	}
	res.Type = entityType;
	++EntityManager.AliveEntities;
	return res;
}

_FORCE_INLINE_ bool IsEntityValid(Entity entity)
{
	SAssert(entity.Id < MAX_ENTITIES);

	if (entity.Id >= MAX_ENTITIES)
		return false;
	
	Entity foundEntity = EntityManager.Entities.AtCopy(entity.Id);
	return (foundEntity.Id == entity.Id && foundEntity.Gen == entity.Gen);
}

inline void DeleteEntity(Entity entity)
{
	if (IsEntityValid(entity))
	{
		for (int i = 0; i < GlobalComponentIdCounter; ++i)
		{
			u32 idx = EntityManager.Components.At(i)->Remove(entity.Id);
			if (idx == UINT32_MAX)
				ArrayRemoveAt(EntityManager.ComponentsValues.At(i), idx);
		}

		EntityManager.Entities.At(entity.Id)->Gen += 1;
		EntityManager.OpenIds.Push(&entity.Id);
		--EntityManager.AliveEntities;
	}
}

template<typename T>
inline T*
GetComponent(Entity entity)
{
	SAssertMsg(EntityManager.ComponentsRegistry.Data[T::Id].Stride > 0, "Component was not registered!");

	if (IsEntityValid(entity))
	{
		int componentId = T::Id;

		u32 entitiesComponentIdx = EntityManager.Components.At(componentId)->Get(entity.Id);
		if (entitiesComponentIdx == UINT32_MAX)
			return nullptr;

		SArray* componentArray = EntityManager.ComponentsValues.At(componentId);
		SAssert(componentArray);
		T* component = ArrayPeekAt(componentArray, entitiesComponentIdx);
		SAssert(component);
		return component;
	}
	else
		return nullptr;
}

template<typename T>
inline T*
AddComponent(Entity entity, T* componentData)
{
	SAssertMsg(EntityManager.ComponentsRegistry.Data[T:Id].Stride > 0, "Component was not registered!");

	if (IsEntityValid(entity))
	{
		int componentId = T::Id;

		u32 entitiesComponentIdx = EntityManager.Components.At(componentId)->Add(entity.Id);
		if (entitiesComponentIdx == UINT32_MAX)
			return nullptr;

		SArray* componentArray = EntityManager.ComponentsValues.At(componentId);
		SAssert(componentArray);
		T* component = ArrayPeekAt(componentArray, entitiesComponentIdx);
		SAssert(component);
		return component;
	}
	else
		return nullptr;
}

template<typename T>
inline void
RemoveComponent(Entity entity)
{
	SAssertMsg(EntityManager.ComponentsRegistry.Data[T:Id].Stride > 0, "Component was not registered!");

	if (IsEntityValid(entity))
	{
		int componentId = T::Id;

		u32 entitiesComponentIdx = EntityManager.Components.At(componentId)->Remove(entity.Id);
		if (entitiesComponentIdx == UINT32_MAX)
			return;

		SArray* componentArray = EntityManager.ComponentsValues.At(componentId);
		SAssert(componentArray);
		ArrayRemoveAt(componentArray, entitiesComponentIdx);
	}
}
