#pragma once

#include "TileMap.h"

#include "Structures/BHeap.h"
#include "Structures/HashMap.h"
#include "Structures/HashSet.h"
#include "Structures/SList.h"
#include "Structures/HashMapT.h"
#include "Structures/HashSetT.h"

constexpr int MAX_PATHFIND_LENGTH = CHUNK_SIZE * 5;

struct Pathfinder
{
	BHeap* Open;
	HashMapT<Vec2i, int> OpenSet;
	HashSetT<Vec2i> ClosedSet;
	//HashMap OpenSet;
	//HashSet ClosedSet;
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

Node* FindPath(Pathfinder* pathfinder, TileMap* tilemap, Vec2i start, Vec2i end);
