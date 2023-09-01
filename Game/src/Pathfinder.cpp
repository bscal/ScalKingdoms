#include "Pathfinder.h"

#include "GameState.h"
#include "Tile.h"

#include <math.h>

constexpr int MAX_SEARCH_TILES = 64 * 64;
constexpr int PATHFINDER_TABLE_SIZE = MAX_SEARCH_TILES + (int)((float)MAX_SEARCH_TILES * HASHSET_LOAD_FACTOR);

#define Hash(pos) zpl_fnv32a(&pos, sizeof(Vec2i))
#define AllocNode() ((Node*)GameMalloc(Allocator::Frame, sizeof(Node)));

internal int 
ManhattanDistance(Vec2i v0, Vec2i v1)
{
	constexpr int MOVE_COST = 10;
	constexpr int DIAGONAL_COST = 14;
	int x = abs(v0.x - v1.x);
	int y = abs(v0.y - v1.y);
	int res = MOVE_COST * (x + y) + (DIAGONAL_COST - 2 * MOVE_COST) * (int)fminf((float)x, (float)y);
	//int res = MOVE_COST * (x + y);
	return res;
}

internal int 
CompareCost(void* cur, void* parent)
{
	Node* nodeCur = (Node*)cur;
	Node* nodeParent = (Node*)parent;

	if (nodeCur->FCost == nodeParent->FCost)
		return (nodeCur->HCost < nodeParent->HCost) ? -1 : 1;
	else if (nodeCur->FCost < nodeParent->FCost)
		return -1;
	else
		return 1;
}

void
PathfinderInit(Pathfinder* pathfinder)
{
	pathfinder->Open = BHeapCreate(Allocator::Arena, CompareCost, MAX_SEARCH_TILES);
	HashMapInitialize(&pathfinder->OpenSet, sizeof(int), PATHFINDER_TABLE_SIZE, Allocator::Arena);
	HashSetInitialize(&pathfinder->ClosedSet, PATHFINDER_TABLE_SIZE, Allocator::Arena);
}

SList<Vec2i>
PathFindArray(Pathfinder* pathfinder, TileMap* tilemap, Vec2i start, Vec2i end)
{
	Node* node = FindPath(pathfinder, tilemap, start, end);
	if (node)
	{
		SList<Vec2i> positions = {};
		positions.Alloc = Allocator::Frame;

		positions.Reserve(10);

		positions.Push(&node->Pos);
		Node* prev = node->Parent;
		while (prev)
		{
			positions.Push(&prev->Pos);
			prev = prev->Parent;
		}
		return positions;
	}
	else
	{
		return {};
	}
}

int PathFindArrayFill(Vec2i* inFillArray, Pathfinder* pathfinder, TileMap* tilemap, Vec2i start, Vec2i end)
{
	Node* node = FindPath(pathfinder, tilemap, start, end);
	if (node)
	{
		int count = 0;
		inFillArray[count++] = node->Pos;

		Node* prev = node->Parent;
		while (prev && count < MAX_PATHFIND_LENGTH)
		{
			inFillArray[count++] = prev->Pos;
			prev = prev->Parent;
		}
		return count;
	}
	else
	{
		inFillArray[0] = end;
		return 0;
	}
}

Node*
FindPath(Pathfinder* pathfinder, TileMap* tilemap, Vec2i start, Vec2i end)
{
	BHeapClear(pathfinder->Open);
	HashMapClear(&pathfinder->OpenSet);
	HashSetClear(&pathfinder->ClosedSet);

	Node* node = AllocNode();
	node->Pos = start;
	node->Parent = nullptr;
	node->GCost = 0;
	node->HCost = ManhattanDistance(start, end);
	node->FCost = node->GCost + node->HCost;

	BHeapPushMin(pathfinder->Open, node, node);
	u32 firstHash = Hash(node->Pos);
	HashMapSet(&pathfinder->OpenSet, firstHash, &node->FCost);

	while (pathfinder->Open->Count > 0)
	{
		BHeapItem item = BHeapPopMin(pathfinder->Open);

		Node* curNode = (Node*)item.User;

		u32 hash = Hash(node->Pos);
		HashMapRemove(&pathfinder->OpenSet, hash);
		HashSetSet(&pathfinder->ClosedSet, hash);

		if (curNode->Pos == end)
		{
			return curNode;
		}
		else
		{
			for (int i = 0; i < ArrayLength(Vec2i_NEIGHTBORS_CORNERS); ++i)
			{
				if (pathfinder->Open->Count >= MAX_SEARCH_TILES)
				{
					SDebugLog("Could not find path");
					return nullptr;
				}

				Vec2i next = curNode->Pos + Vec2i_NEIGHTBORS_CORNERS[i];

				u32 nextHash = Hash(next);
				if (HashSetContains(&pathfinder->ClosedSet, nextHash))
					continue;

				Tile* tile = GetTile(tilemap, next);
				if (!tile || tile->Flags.Get(TILE_FLAG_COLLISION))
					continue;
				else
				{
					int* nextCost = HashMapGet(&pathfinder->OpenSet, nextHash, int);
					int tileCost = GetTileInfo(tile->BackgroundId)->MovementCost;
					int cost = curNode->GCost + ManhattanDistance(curNode->Pos, next) + tileCost;
					if (!nextCost || cost < *nextCost)
					{
						Node* nextNode = AllocNode();
						nextNode->Pos = next;
						nextNode->Parent = curNode;
						nextNode->GCost = cost;
						nextNode->HCost = ManhattanDistance(end, next);
						nextNode->FCost = nextNode->GCost + nextNode->HCost;

						BHeapPushMin(pathfinder->Open, nextNode, nextNode);
						if (!nextCost)
						{
							HashMapSet(&pathfinder->OpenSet, nextHash, &nextNode->GCost);
						}
						else
						{
							*nextCost = nextNode->GCost;
						}
					}
				}
			}
		}
	}
	return nullptr;
}
