#pragma once

#include "TileMap.h"

#include "Structures/BHeap.h"
#include "Structures/HashMap.h"
#include "Structures/HashSet.h"
#include "Structures/SList.h"

constexpr int MAX_SEARCH_TILES = 64 * 64;

struct Pathfinder
{
	BHeap* Open;
	HashMap OpenSet;
	HashSet ClosedSet;
};

struct Node
{
	Vec2i Pos;
	Node* Parent;
	int FCost;
	int HCost;
	int GCost;
};

void PathfinderInit(Pathfinder* pathfinder);

Vec2i PathFindNext(Pathfinder* pathfinder, TileMap* tilemap, Vec2i start, Vec2i end);

SList<Vec2i> PathFindArray(Pathfinder* pathfinder, TileMap* tilemap, Vec2i start, Vec2i end);
