#include "Pathfinder.h"

#include "GameState.h"

void
PathfinderInit(Pathfinder* pathfinder)
{
	pathfinder->Open = BHeapCreate(ALLOCATOR_HEAP, CompareCost, DIMENSIONS * DIMENSIONS);
	pathfinder->OpenSet.Initialize(DIMENSIONS * DIMENSIONS, ALLOCATOR_HEAP);
	pathfinder->ClosedSet.Initialize(DIMENSIONS * DIMENSIONS, ALLOCATOR_HEAP);
}

Vec2i
PathFindNext(Pathfinder* pathfinder, TileMap* tilemap, Vec2i start, Vec2i end)
{
	BHeapClear(pathfinder->Open);
	pathfinder->OpenSet.Clear();
	pathfinder->ClosedSet.Clear();

	Node* node = AllocNode();
	node->Pos = start;
	node->DistFromStart = 0;
	node->DistFromEnd = start.Distance(end);
	node->Cost = node->DistFromEnd;
	node->Parent = nullptr;

	BHeapPushMin(pathfinder->Open, node, node);
	u32 firstHash = Hash(node->Pos);
	pathfinder->OpenSet.Put(firstHash);

	while (pathfinder->Open->Count > 0)
	{
		BHeapItem item = BHeapPopMin(pathfinder->Open);

		Node* node = (Node*)item.User;

		u32 hash = Hash(node->Pos);
		pathfinder->ClosedSet.Put(hash);

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
				if (pathfinder->ClosedSet.Contains(nextHash))
					continue;

				Tile* tile = GetTile(tilemap, next);
				if (tile->Flags.Get(TILE_FLAG_SOLID))
					continue;
				else
				{
					float distance = next.Distance(end);
					float startDist = next.Distance(start);
					float total = startDist + distance;
					if (total < node->Cost || !pathfinder->OpenSet.Contains(nextHash))
					{
						Node* nextNode = AllocNode();
						nextNode->Pos = next;
						nextNode->DistFromStart = startDist;
						nextNode->DistFromEnd = distance;
						nextNode->Parent = node;
						nextNode->Cost = total;

						BHeapPushMin(pathfinder->Open, nextNode, nextNode);
						pathfinder->OpenSet.Put(nextHash);
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
	pathfinder->OpenSet.Clear();
	pathfinder->ClosedSet.Clear();

	Node* node = AllocNode();
	node->Pos = start;
	node->DistFromStart = 0;
	node->DistFromEnd = start.Distance(end);
	node->Cost = node->DistFromEnd;
	node->Parent = nullptr;

	BHeapPushMin(pathfinder->Open, node, node);
	u32 firstHash = Hash(node->Pos);
	pathfinder->OpenSet.Put(firstHash);

	while (pathfinder->Open->Count > 0)
	{
		BHeapItem item = BHeapPopMin(pathfinder->Open);

		Node* node = (Node*)item.User;

		u32 hash = Hash(node->Pos);
		pathfinder->ClosedSet.Put(hash);

		if (node->Pos == end)
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
			return positions;
		}
		else
		{
			for (int i = 0; i < ArrayLength(Vec2i_NEIGHTBORS_CORNERS); ++i)
			{
				Vec2i next = node->Pos + Vec2i_NEIGHTBORS_CORNERS[i];
				u32 nextHash = Hash(next);
				if (pathfinder->ClosedSet.Contains(nextHash))
					continue;

				Tile* tile = GetTile(tilemap, next);
				if (!tile || tile->Flags.Get(TILE_FLAG_SOLID))
					continue;
				else
				{
					float distance = next.Distance(end);
					if (distance < node->DistFromEnd || !pathfinder->OpenSet.Contains(nextHash))
					{
						Node* nextNode = AllocNode();
						nextNode->Pos = next;
						nextNode->DistFromStart = next.Distance(start);
						nextNode->DistFromEnd = distance;
						nextNode->Parent = node;
						nextNode->Cost = nextNode->DistFromStart + nextNode->DistFromEnd;

						BHeapPushMin(pathfinder->Open, nextNode, nextNode);
						pathfinder->OpenSet.Put(nextHash);
					}
				}
			}
		}
	}
	return {};
}


