#pragma once

#include "TileMap.h"

#include "Structures/BHeap.h"
#include "Structures/HashSet.h"
#include "Structures/SList.h"

constexpr int DIMENSIONS = 128;

struct Pathfinder
{
	BHeap* Open;
	HashSet OpenSet;
	HashSet ClosedSet;
};

struct Node
{
	Vec2i Pos;
	float Cost;
	float DistFromStart;
	float DistFromEnd;
	Node* Parent;
};

#define Hash(pos) zpl_fnv32a(&pos, sizeof(Vec2i))
#define AllocNode() ((Node*)AllocFrame(sizeof(Node), 16));

internal int CompareCost(void* v0, void* v1)
{
	Node* node0 = (Node*)v0;
	Node* node1 = (Node*)v1;
	bool lessThan = node0->Cost < node1->Cost;
	return (lessThan) ? -1 : 1;
}

void PathfinderInit(Pathfinder* pathfinder);

Vec2i PathFindNext(Pathfinder* pathfinder, TileMap* tilemap, Vec2i start, Vec2i end);

SList<Vec2i> PathFindArray(Pathfinder* pathfinder, TileMap* tilemap, Vec2i start, Vec2i end);
