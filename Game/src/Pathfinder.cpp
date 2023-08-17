#include "Pathfinder.h"

#include "GameState.h"
#include "Tile.h"

#include <math.h>

#define Hash(pos) zpl_fnv32a(&pos, sizeof(Vec2i))
#define AllocNode() ((Node*)AllocFrame(sizeof(Node), 16));

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
	constexpr int HASH_SIZE = MAX_SEARCH_TILES + (int)((float)MAX_SEARCH_TILES * HASHSET_LOAD_FACTOR);
	pathfinder->Open = BHeapCreate(ALLOCATOR_HEAP, CompareCost, MAX_SEARCH_TILES);
	HashMapInitialize(&pathfinder->OpenSet, sizeof(int), HASH_SIZE, ALLOCATOR_HEAP);
	HashSetInitialize(&pathfinder->ClosedSet, HASH_SIZE, ALLOCATOR_HEAP);
}

Vec2i
PathFindNext(Pathfinder* pathfinder, TileMap* tilemap, Vec2i start, Vec2i end)
{
	BHeapClear(pathfinder->Open);
	HashMapClear(&pathfinder->OpenSet);
	HashSetClear(&pathfinder->ClosedSet);

	Node* node = AllocNode();
	node->Pos = start;
	node->Parent = nullptr;
	node->HCost = 0;
	node->GCost = ManhattanDistance(start, end);
	node->FCost = node->GCost;

	BHeapPushMin(pathfinder->Open, node, node);
	u32 firstHash = Hash(node->Pos);
	HashMapSet(&pathfinder->OpenSet, firstHash, node->FCost, int);

	while (pathfinder->Open->Count > 0)
	{
		BHeapItem item = BHeapPopMin(pathfinder->Open);

		Node* node = (Node*)item.User;

		u32 hash = Hash(node->Pos);
		HashMapRemove(&pathfinder->OpenSet, hash);
		HashSetPut(&pathfinder->ClosedSet, hash);

		if (node->Pos == end)
		{
			Vec2i res = node->Pos;
			Node* prev = node->Parent;
			while (prev)
			{
				res = prev->Pos;
				prev = prev->Parent;
			}
			return res;
		}
		else
		{
			for (int i = 0; i < ArrayLength(Vec2i_NEIGHTBORS_CORNERS); ++i)
			{
				Vec2i next = node->Pos + Vec2i_NEIGHTBORS_CORNERS[i];
				u32 nextHash = Hash(node->Pos);
				if (HashSetContains(&pathfinder->ClosedSet, nextHash))
					continue;

				Tile* tile = GetTile(tilemap, next);
				if (tile->Flags.Get(TILE_FLAG_SOLID))
					continue;
				else
				{
					int* nextCost = HashMapGet(&pathfinder->OpenSet, nextHash, int);
					int distance = ManhattanDistance(next, end);
					if (distance < *nextCost || !nextCost)
					{
						Node* nextNode = AllocNode();
						nextNode->Pos = next;
						nextNode->Parent = node;
						nextNode->HCost = ManhattanDistance(next, start);
						nextNode->GCost = distance;
						nextNode->FCost = nextNode->HCost + nextNode->GCost;

						if (!nextCost)
						{
							BHeapPushMin(pathfinder->Open, nextNode, nextNode);
							HashMapSet(&pathfinder->OpenSet, nextHash, nextNode->FCost, int);
						}
					}
				}
			}
		}
	}
	return start;
}

SList<Vec2i>
PathFindArray(Pathfinder* pathfinder, TileMap* tilemap, Vec2i start, Vec2i end)
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
	HashMapSet(&pathfinder->OpenSet, firstHash, node->FCost, int);

	while (pathfinder->Open->Count > 0)
	{
		BHeapItem item = BHeapPopMin(pathfinder->Open);

		Node* node = (Node*)item.User;

		u32 hash = Hash(node->Pos);
		HashMapRemove(&pathfinder->OpenSet, hash);
		HashSetPut(&pathfinder->ClosedSet, hash);

		if (pathfinder->ClosedSet.Count >= MAX_SEARCH_TILES)
		{
			SLOG_DEBUG("Could not find path");
			return {};
		}
		else if (node->Pos == end)
		{
			SList<Vec2i> positions = {};
			positions.Alloc = ALLOCATOR_FRAME;

			positions.Reserve(10);

			positions.Push(&node->Pos);
			Node* prev = node->Parent;
			while (prev)
			{
				positions.Push(&prev->Pos);
				prev = prev->Parent;
			}
			SLOG_DEBUG("Checked %d tiles", pathfinder->ClosedSet.Count);
			return positions;
		}
		else
		{
			for (int i = 0; i < ArrayLength(Vec2i_NEIGHTBORS_CORNERS); ++i)
			{
				Vec2i next = node->Pos + Vec2i_NEIGHTBORS_CORNERS[i];

				u32 nextHash = Hash(next);
				if (HashSetContains(&pathfinder->ClosedSet, nextHash))
					continue;

				Tile* tile = GetTile(tilemap, next);
				if (!tile || tile->Flags.Get(TILE_FLAG_SOLID))
					continue;
				else
				{
					int* nextCost = HashMapGet(&pathfinder->OpenSet, nextHash, int);
					int tileCost = GetTileInfo(tile->BackgroundId)->MovementCost;
					int cost = node->GCost + ManhattanDistance(node->Pos, next) + tileCost;
					if (!nextCost || cost < *nextCost)
					{
						Node* nextNode = AllocNode();
						nextNode->Pos = next;
						nextNode->Parent = node;
						nextNode->GCost = cost;
						nextNode->HCost = ManhattanDistance(end, next);
						nextNode->FCost = nextNode->GCost + nextNode->HCost;

						if (pathfinder->Open->Count >= MAX_SEARCH_TILES)
						{
							SLOG_DEBUG("Could not find path");
							return {};
						}

						BHeapPushMin(pathfinder->Open, nextNode, nextNode);
						if (!nextCost)
						{
							HashMapSet(&pathfinder->OpenSet, nextHash, nextNode->GCost, int);
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
	return {};
}


