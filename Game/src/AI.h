#pragma once

#include "Core.h"
#include "Structures/StaticArray.h"
#include "Structures/HashMapStr.h"

struct Goal;
struct Brain;

enum VariantType : int
{
	Variant_Vec2,
	Variant_Vec2i,
	Variant_Entity,
	Variant_Pointer,
	Variant_Float,
	Variant_Int,
	Variant_UShort,
	Variant_Bool
};

struct Variant
{
	union
	{
		Vec2 Vec2;
		Vector2i Vec2i;
		ecs_entity_t Entity;
		void* Pointer;
		float Float;
		int Int;
		u16 UShort;
		bool Bool;
	};
	VariantType Type;
};

constant_var size_t INFLUENCE_MAP_SIZE = 32 * 32;
constant_var size_t GOALS_MAX_VALUE = 32;

struct Brain
{
	ecs_entity_t Entity;
	HashMapStr<Variant> Variables;
	Buffer<Goal, GOALS_MAX_VALUE> Goals;
};

struct Goal
{
	float(*GetWeight)(Brain*, GameState*);
	ecs_entity_t(*GetTargets)(Brain*, GameState*);
};



struct InfluenceMap
{
	StaticArray<float, INFLUENCE_MAP_SIZE> Values;
};
