#pragma once

#include "Core.h"
#include "Structures/HashMap.h"
#include "Structures/ArrayList.h"
#include "Structures/HashMapT.h"

struct Stockpile
{
	HashMap Quantities;
	ArrayList(Vec2i) Tiles;
};

struct ItemStackHolder
{
	HashMapT<u16, u16> Quantities;
	ArrayList(ecs_entity_t) ItemStacks;
};