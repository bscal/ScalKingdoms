#include "Regions.h"

#include "GameState.h"
#include "Components.h"
#include "TileMap.h"
#include "Tile.h"
#include "Pathfinder.h"

#define MAX_REGION_LENGTH 64
#define Hash(pos) zpl_fnv32a(&pos, sizeof(Vec2i))
#define AllocNode(T) ((T*)AllocFrame(sizeof(T), 16));

internal inline int
RegionCompareCost(void* cur, void* parent)
{
	RegionNode* a = (RegionNode*)cur;
	RegionNode* b = (RegionNode*)parent;
	if (a->FCost == b->FCost)
		return (a->HCost < b->HCost) ? -1 : 1;
	else if (a->FCost < b->FCost)
		return -1;
	else
		return 1;
}

internal int 
ManhattanDistance(Vec2i v0, Vec2i v1)
{
	constexpr int MOVE_COST = 10;
	constexpr int DIAGONAL_COST = 14;
	int x = abs(v0.x - v1.x);
	int y = abs(v0.y - v1.y);
	int res = MOVE_COST * (x + y);
	return res;
}

void RegionStateInit(RegionState* regionState)
{
	BHeapCreate(zpl_heap_allocator(), RegionCompareCost, MAX_REGION_LENGTH);
	HashMapInitialize(&regionState->RegionMap, sizeof(RegionPaths), 8 * 8, zpl_heap_allocator());
	HashMapInitialize(&regionState->OpenMap, sizeof(RegionNode*), MAX_REGION_LENGTH, zpl_heap_allocator());
	HashSetInitialize(&regionState->ClosedSet, MAX_REGION_LENGTH, zpl_heap_allocator());
}

void 
LoadRegionPaths(RegionState* regionState, RegionPaths* regionPath, TileMap* tilemap, Chunk* chunk)
{
	SASSERT(regionPath);
	SASSERT(tilemap);
	SASSERT(chunk);

	regionPath->Nodes[0] = Vec2i_NULL;
	regionPath->Nodes[1] = Vec2i_NULL;
	regionPath->Nodes[2] = Vec2i_NULL;
	regionPath->Nodes[3] = Vec2i_NULL;

	Vec2i chunkWorld = ChunkToTile(chunk->Coord);

	for (int i = 0; i < REGION_SIZE; ++i)
	{
		Vec2i tile = chunkWorld + Vec2i{ i, 0 };
		Vec2i borderTile = tile + Vec2i_UP;

		Tile* thisTile = GetTile(tilemap, tile);
		if (thisTile->Flags.Get(TILE_FLAG_COLLISION))
			continue;

		Tile* otherTile = GetTile(tilemap, tile);
		if (otherTile->Flags.Get(TILE_FLAG_COLLISION))
			continue;

		regionPath->Nodes[0] = tile;
	}

	for (int i = 0; i < REGION_SIZE; ++i)
	{
		Vec2i tile = chunkWorld + Vec2i{ REGION_SIZE, i };
		Vec2i borderTile = tile + Vec2i_RIGHT;

		Tile* thisTile = GetTile(tilemap, tile);
		if (thisTile->Flags.Get(TILE_FLAG_COLLISION))
			continue;

		Tile* otherTile = GetTile(tilemap, tile);
		if (otherTile->Flags.Get(TILE_FLAG_COLLISION))
			continue;

		regionPath->Nodes[1] = tile;
	}

	for (int i = 0; i < REGION_SIZE; ++i)
	{
		Vec2i tile = chunkWorld + Vec2i{ i, REGION_SIZE };
		Vec2i borderTile = tile + Vec2i_DOWN;

		Tile* thisTile = GetTile(tilemap, tile);
		if (thisTile->Flags.Get(TILE_FLAG_COLLISION))
			continue;

		Tile* otherTile = GetTile(tilemap, tile);
		if (otherTile->Flags.Get(TILE_FLAG_COLLISION))
			continue;

		regionPath->Nodes[2] = tile;
	}

	for (int i = 0; i < REGION_SIZE; ++i)
	{
		Vec2i tile = chunkWorld + Vec2i{ 0, REGION_SIZE };
		Vec2i borderTile = tile + Vec2i_LEFT;

		Tile* thisTile = GetTile(tilemap, tile);
		if (thisTile->Flags.Get(TILE_FLAG_COLLISION))
			continue;

		Tile* otherTile = GetTile(tilemap, tile);
		if (otherTile->Flags.Get(TILE_FLAG_COLLISION))
			continue;

		regionPath->Nodes[3] = tile;
	}

	regionPath->NeightborChunks[0] = chunk->Coord + Vec2i_UP;
	regionPath->NeightborChunks[1] = chunk->Coord + Vec2i_RIGHT;
	regionPath->NeightborChunks[2] = chunk->Coord + Vec2i_DOWN;
	regionPath->NeightborChunks[3] = chunk->Coord + Vec2i_LEFT;

	u32 hash = HashTile(chunk->Coord);
	HashMapSet(&regionState->RegionMap, hash, regionPath);
}

void
FindRegionPath(RegionState* regionState, TileMap* tilemap, Vec2i start, Vec2i end, zpl_array(int) arr)
{
	zpl_array_clear(arr);

	RegionPaths* startRegion = GetRegion(regionState, start);
	RegionPaths* endRegion = GetRegion(regionState, end);

	if (startRegion == endRegion)
	{
		return;
	}
	else
	{
		BHeapClear(regionState->Open);
		HashMapClear(&regionState->OpenMap);
		HashSetClear(&regionState->ClosedSet);

		RegionNode* node = AllocNode(RegionNode);
		node->Pos = start;
		node->Parent = nullptr;
		node->GCost = 0;
		node->HCost = ManhattanDistance(start, end);
		node->FCost = node->GCost + node->HCost;

		BHeapPushMin(regionState->Open, node, node);
		u32 firstHash = Hash(node->Pos);
		HashMapSet(&regionState->OpenMap, firstHash, &node->FCost);

		while (regionState->Open->Count > 0)
		{
			BHeapItem item = BHeapPopMin(regionState->Open);

			RegionNode* node = (RegionNode*)item.User;
			RegionPaths* curRegion = GetRegion(regionState, node->Pos);

			u32 hash = Hash(node->Pos);
			HashMapRemove(&regionState->OpenMap, hash);
			HashSetSet(&regionState->ClosedSet, hash);

			if (node->Pos == end)
			{
				int count = 0;
				zpl_array_clear(arr);
				
				++count;
				zpl_array_append(arr, node->MoveDirection);

				RegionNode* prev = node->Parent;
				while (prev)
				{
					++count;
					zpl_array_append(arr, prev->MoveDirection);
					prev = prev->Parent;
				}
			}
			else
			{
				for (int i = 0; i < ArrayLength(Vec2i_NEIGHTBORS); ++i)
				{
					if (regionState->Open->Count >= MAX_REGION_LENGTH)
					{
						SLOG_DEBUG("Could not find path");
						return;
					}

					Vec2i next = node->Pos + Vec2i_NEIGHTBORS[i];

					if (curRegion->Nodes[i] == Vec2i_NULL)
						continue;

					u32 nextHash = Hash(next);
					if (HashSetContains(&regionState->ClosedSet, nextHash))
						continue;

					RegionPaths* nextRegion = GetRegion(regionState, next);
					if (!nextRegion)
						continue;
					else
					{
						int* nextCost = HashMapGet(&regionState->OpenMap, nextHash, int);
						int cost = node->GCost + ManhattanDistance(node->Pos, next) + nextRegion->ChunkCost;
						if (!nextCost || cost < *nextCost)
						{
							RegionNode* nextNode = AllocNode(RegionNode);
							nextNode->Pos = next;
							nextNode->Parent = node;
							nextNode->GCost = cost;
							nextNode->HCost = ManhattanDistance(end, next);
							nextNode->FCost = nextNode->GCost + nextNode->HCost;
							nextNode->MoveDirection = i;

							BHeapPushMin(regionState->Open, nextNode, nextNode);
							if (!nextCost)
							{
								HashMapSet(&regionState->OpenMap, nextHash, &nextNode->GCost);
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
	}
}

Vec2i 
RegionGetNode(RegionState* regionState, Vec2i regionPos, int direction)
{
	RegionPaths* nextRegion = GetRegion(regionState, regionPos + Vec2i_NEIGHTBORS[direction]);
	
	constexpr static int INVERSE_DIRECTIONS[] = { 3, 2, 1, 0 };
	Vec2i node = nextRegion->Nodes[INVERSE_DIRECTIONS[direction]];
	return node;
}

void 
RegionFindLocalPath(RegionState* regionState, Vec2i start, Vec2i end, zpl_array(Vec2i) arr)
{
	Node* node = FindPath(&GetGameState()->Pathfinder, &GetGameState()->TileMap, start, end);

	zpl_array_clear(arr);
	
	if (node)
	{
		int count = 0;
		
		zpl_array_append(arr, node->Pos);
		++count;

		Node* prev = node->Parent;
		while (prev && count < 256)
		{
			zpl_array_append(arr, prev->Pos);
			prev = prev->Parent;
			++count;
		}
		if (Client.TileMapDebugFlag.Get(TILE_MAP_DEBUG_FLAG_PATHFINDING))
		{
			//zpl_array_clear(Client.PathfinderPath);
			//for (int i = 0; i < count; ++i)
				//zpl_array_append(Client.PathfinderPath, inFillArray[i]);
			SLOG_DEBUG("Checked %d tiles, length %d tiles", GetGameState()->Pathfinder.ClosedSet.Count, count);
		}
	}
}

RegionPaths* 
GetRegion(RegionState* regionState, Vec2i tilePos)
{
	int regionX = tilePos.x / REGION_SIZE;
	int regionY = tilePos.y / REGION_SIZE;
	Vec2i coord = Vec2i{ regionX, regionY };
	return HashMapGet(&regionState->RegionMap, HashTile(coord), RegionPaths);

}
