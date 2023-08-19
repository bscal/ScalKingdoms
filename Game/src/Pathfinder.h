#pragma once

#include "TileMap.h"

#include "Structures/BHeap.h"
#include "Structures/HashMap.h"
#include "Structures/HashSet.h"
#include "Structures/SList.h"

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

SList<Vec2i> PathFindArray(Pathfinder* pathfinder, TileMap* tilemap, Vec2i start, Vec2i end);

int PathFindArrayFill(Vec2i* inFillArray, Pathfinder* pathfinder, TileMap* tilemap, Vec2i start, Vec2i end);
