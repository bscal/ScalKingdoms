#include "Regions.h"

#include "GameState.h"
#include "Components.h"
#include "TileMap.h"
#include "Tile.h"

#include "Structures/ArrayList.h"
#include <corecrt_math.h>

constexpr global_var int MAX_REGION_SEARCH = 128;
constexpr global_var int MAX_LOCAL_SEARCH = 256;

#define HashRegion(coord) (zpl_fnv64a(&coord, sizeof(Vec2i)))
#define AllocNode(T) ((T*)GameMalloc(Allocator::Frame, sizeof(T)));

struct RegionNode
{
	Vec2i Pos;
	RegionNode* Parent;
	int FCost;
	int HCost;
	int GCost;
};

internal _FORCE_INLINE_ Vec2i
TileCoordToRegionCoord(Vec2i tile)
{
	Vec2i res;
	res.x = (int)floorf((float)tile.x / REGION_SIZE);
	res.y = (int)floorf((float)tile.y / REGION_SIZE);
	return res;
}

internal int
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
CalculateDistance(Vec2i v0, Vec2i v1)
{
	constexpr int MOVE_COST = 10;
	constexpr int DIAGONAL_COST = 14;
	int x = abs(v0.x - v1.x);
	int y = abs(v0.y - v1.y);
	int res = MOVE_COST * (x + y) + (DIAGONAL_COST - 2 * MOVE_COST) * (int)fminf((float)x, (float)y);
	return res;
}

internal bool
RegionCompareFunc(const void* v0, const void* v1)
{
	Vec2i* vec0 = (Vec2i*)v0;
	Vec2i* vec1 = (Vec2i*)v1;
	return *vec0 == *vec1;
}

void 
RegionStateInit(RegionState* regionState)
{
	regionState->Open = BHeapCreate(Allocator::Arena, RegionCompareCost, MAX_REGION_SEARCH);
	HashMapKVInitialize(&regionState->RegionMap, RegionCompareFunc, sizeof(Vec2i), sizeof(RegionPaths), MAX_REGION_SEARCH, Allocator::Arena);
	HashMapTInitialize(&regionState->OpenMap, sizeof(RegionNode*), MAX_REGION_SEARCH, Allocator::Arena);
	HashSetTInitialize(&regionState->ClosedSet, MAX_REGION_SEARCH, Allocator::Arena);
}

void 
LoadRegionPaths(RegionState* regionState, TileMap* tilemap, Chunk* chunk)
{
	SASSERT(regionState);
	SASSERT(tilemap);
	SASSERT(chunk);
	
	Vec2i chunkWorld = ChunkToTile(chunk->Coord);

	SInfoLog("Loading regions for %s", FMT_VEC2I(chunk->Coord));

	for (int yDiv = 0; yDiv < DIVISIONS; ++yDiv)
	{
		for (int xDiv = 0; xDiv < DIVISIONS; ++xDiv)
		{
			RegionPaths region = {};

			Vec2i pos = chunkWorld + Vec2i{ xDiv * REGION_SIZE, yDiv * REGION_SIZE };
			region.TilePos = pos;
			region.Pos = { pos.x / REGION_SIZE, pos.y / REGION_SIZE };

			for (int i = 0; i < REGION_SIZE; ++i)
			{
				Vec2i tilePos = pos + Vec2i{ i, 0 };
				Vec2i neighborTilePos = tilePos + Vec2i_NEIGHTBORS[0];

				Tile* tile = GetTile(tilemap, tilePos);
				if (!tile || tile->Flags.Get(TILE_FLAG_COLLISION))
					continue;

				Tile* neighborTile = GetTile(tilemap, neighborTilePos);
				if (!neighborTile || neighborTile->Flags.Get(TILE_FLAG_COLLISION))
					continue;

				region.Sides[0] = true;
				break;
			}


			for (int i = 0; i < REGION_SIZE; ++i)
			{
				Vec2i tilePos = pos + Vec2i{ REGION_SIZE - 1, i };
				Vec2i neighborTilePos = tilePos + Vec2i_NEIGHTBORS[1];

				Tile* tile = GetTile(tilemap, tilePos);
				if (!tile || tile->Flags.Get(TILE_FLAG_COLLISION))
					continue;

				Tile* neighborTile = GetTile(tilemap, neighborTilePos);
				if (!neighborTile || neighborTile->Flags.Get(TILE_FLAG_COLLISION))
					continue;

				region.Sides[1] = true;
				break;
			}


			for (int i = 0; i < REGION_SIZE; ++i)
			{
				Vec2i tilePos = pos + Vec2i{ i, REGION_SIZE - 1 };
				Vec2i neighborTilePos = tilePos + Vec2i_NEIGHTBORS[2];

				Tile* tile = GetTile(tilemap, tilePos);
				if (!tile || tile->Flags.Get(TILE_FLAG_COLLISION))
					continue;

				Tile* neighborTile = GetTile(tilemap, neighborTilePos);
				if (!neighborTile || neighborTile->Flags.Get(TILE_FLAG_COLLISION))
					continue;

				region.Sides[2] = true;
				break;
			}


			for (int i = 0; i < REGION_SIZE; ++i)
			{
				Vec2i tilePos = pos + Vec2i{ 0, i };
				Vec2i neighborTilePos = tilePos + Vec2i_NEIGHTBORS[3];

				Tile* tile = GetTile(tilemap, tilePos);
				if (!tile || tile->Flags.Get(TILE_FLAG_COLLISION))
					continue;

				Tile* neighborTile = GetTile(tilemap, neighborTilePos);
				if (!neighborTile || neighborTile->Flags.Get(TILE_FLAG_COLLISION))
					continue;

				region.Sides[3] = true;
				break;
			}

			HashMapKVReplace(&regionState->RegionMap, &region.Pos, &region);
		}
	}
}

void 
UnloadRegionPaths(RegionState* regionState, Chunk* chunk)
{
	SASSERT(regionState);
	SASSERT(chunk);

	SInfoLog("Unloading regions for %s", FMT_VEC2I(chunk->Coord));

	Vec2i chunkWorld = ChunkToTile(chunk->Coord);
	Vec2i startRegionCoord = TileCoordToRegionCoord(chunkWorld);
	for (int yDiv = 0; yDiv < DIVISIONS; ++yDiv)
	{
		for (int xDiv = 0; xDiv < DIVISIONS; ++xDiv)
		{
			Vec2i pos = startRegionCoord + Vec2i{ xDiv, yDiv };
			HashMapKVRemove(&regionState->RegionMap, &pos);
		}
	}
}

void
FindRegionPath(RegionState* regionState, TileMap* tilemap, Vec2i startTile, Vec2i endTile, HashSetT<Vec2i>* regionSet)
{
	SASSERT(regionState);
	SASSERT(tilemap);
	SASSERT(regionSet);

	Vec2i start = TileCoordToRegionCoord(startTile);
	Vec2i end = TileCoordToRegionCoord(endTile);

	HashSetTClear(regionSet);

	BHeapClear(regionState->Open);
	HashMapTClear(&regionState->OpenMap);
	HashSetTClear(&regionState->ClosedSet);

	RegionNode* node = AllocNode(RegionNode);
	node->Pos = start;
	node->Parent = nullptr;
	node->GCost = 0;
	node->HCost = CalculateDistance(start, end);
	node->FCost = node->GCost + node->HCost;

	BHeapPushMin(regionState->Open, node, node);
	HashMapTSet(&regionState->OpenMap, &node->Pos, &node->FCost);

	while (regionState->Open->Count > 0)
	{
		BHeapItem item = BHeapPopMin(regionState->Open);

		RegionNode* curNode = (RegionNode*)item.User;

		//u64 hash = HashRegion(curNode->Pos);
		bool wasRemoved = HashMapTRemove(&regionState->OpenMap, &curNode->Pos);
		SASSERT(wasRemoved);
		HashSetTSet(&regionState->ClosedSet, &curNode->Pos);

		if (curNode->Pos == end)
		{
			if (Client.DebugShowRegionPaths)
				ArrayListClear(Client.DebugRegionsPath);

			RegionNode* prev = curNode;
			while (prev)
			{
				if (Client.DebugShowRegionPaths)
					ArrayListPush(Allocator::Arena, Client.DebugRegionsPath, prev->Pos);

				//u64 prevHash = HashTile(prev->Pos);
				HashSetTSet(regionSet, &prev->Pos);
				prev = prev->Parent;
			}

			if (Client.DebugShowRegionPaths)
				SDebugLog("[ Debug::Pathfinding ] %d region path length, %d total regions", regionSet->Count, regionState->ClosedSet.Count);

			return;
		}
		else
		{
			for (size_t i = 0; i < ArrayLength(Vec2i_NEIGHTBORS); ++i)
			{
				if (regionState->Open->Count >= MAX_REGION_SEARCH)
				{
					SDebugLog("Could not find region path!");
					return;
				}

				Vec2i next = curNode->Pos + Vec2i_NEIGHTBORS[i];

				//u64 nextHash = HashRegion(next);
				if (HashSetTContains(&regionState->ClosedSet, &next))
					continue;

				RegionPaths* nextRegion = (RegionPaths*)HashMapKVGet(&regionState->RegionMap, &next);
				if (!nextRegion || !nextRegion->Sides[i])
					continue;
				else
				{
					int* nextCost = HashMapTGet(&regionState->OpenMap, &next);
					int cost = curNode->GCost + CalculateDistance(curNode->Pos, next);
					if (!nextCost || cost < *nextCost)
					{
						RegionNode* nextNode = AllocNode(RegionNode);
						nextNode->Pos = next;
						nextNode->Parent = curNode;
						nextNode->GCost = cost;
						nextNode->HCost = CalculateDistance(end, next);
						nextNode->FCost = nextNode->GCost + nextNode->HCost;

						
						if (!nextCost)
						{
							BHeapPushMin(regionState->Open, nextNode, nextNode);
							HashMapTSet(&regionState->OpenMap, &next, &nextNode->GCost);
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

	if (!regionSet->Count)
	{
		SDebugLog("Could not find region path!");
	}
}

void 
RegionFindLocalPath(RegionState* regionState, Vec2i start, Vec2i end, CMove* moveComponent)
{
	Pathfinder* pathfinder = &GetGameState()->Pathfinder;
	TileMap* tilemap = &GetGameState()->TileMap;

	zpl_array_clear(moveComponent->TilePath);

	BHeapClear(pathfinder->Open);
	HashMapTClear(&pathfinder->OpenSet);
	HashSetTClear(&pathfinder->ClosedSet);

	Node* node = AllocNode(Node);
	node->Pos = start;
	node->Parent = nullptr;
	node->GCost = 0;
	node->HCost = CalculateDistance(start, end);
	node->FCost = node->GCost + node->HCost;

	BHeapPushMin(pathfinder->Open, node, node);
	//u64 firstHash = HashRegion(node->Pos);
	HashMapTSet(&pathfinder->OpenSet, &node->Pos, &node->FCost);

	while (pathfinder->Open->Count > 0)
	{
		BHeapItem item = BHeapPopMin(pathfinder->Open);

		Node* curNode = (Node*)item.User;

		//u64 hash = HashRegion(curNode->Pos);
		HashMapTRemove(&pathfinder->OpenSet, &curNode->Pos);
		HashSetTSet(&pathfinder->ClosedSet, &curNode->Pos);

		if (curNode->Pos == end)
		{
			int count = 0;

			Node* prev = curNode;
			while (prev)
			{
				zpl_array_append(moveComponent->TilePath, prev->Pos);
				prev = prev->Parent;
				++count;
			}
			SDebugLog("[ Debug::Pathfinding ] %d path length, %d total tiles", count, GetGameState()->Pathfinder.ClosedSet.Count);
		}
		else
		{
			for (size_t i = 0; i < ArrayLength(Vec2i_NEIGHTBORS_CORNERS); ++i)
			{
				Vec2i nextTile = curNode->Pos + Vec2i_NEIGHTBORS_CORNERS[i];

				RegionPaths* nextRegion = GetRegion(regionState, nextTile);
				//u64 nextRegionhash = HashRegion(nextRegion->Pos);
				if (!nextRegion || !HashSetTContains(&moveComponent->Regions, &nextRegion->Pos))
					continue;

				//u64 nextHash = HashTile(nextTile);
				if (HashSetTContains(&pathfinder->ClosedSet, &nextTile))
					continue;

				Tile* tile = GetTile(tilemap, nextTile);
				if (!tile || tile->Flags.Get(TILE_FLAG_COLLISION))
					continue;
				else
				{
					int* nextCost = HashMapTGet(&pathfinder->OpenSet, &nextTile);
					int tileCost = GetTileInfo(tile->BackgroundId)->MovementCost;
					int cost = curNode->GCost + CalculateDistance(curNode->Pos, nextTile) + tileCost;
					if (!nextCost || cost < *nextCost)
					{
						Node* nextNode = AllocNode(Node);
						nextNode->Pos = nextTile;
						nextNode->Parent = curNode;
						nextNode->GCost = cost;
						nextNode->HCost = CalculateDistance(end, nextTile);
						nextNode->FCost = nextNode->GCost + nextNode->HCost;

						BHeapPushMin(pathfinder->Open, nextNode, nextNode);
						if (!nextCost)
						{
							HashMapTSet(&pathfinder->OpenSet, &nextTile, &nextNode->GCost);
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

	if (zpl_array_count(moveComponent->TilePath) == 0)
	{
		SDebugLog("Could not find local tile path!");
	}
}

RegionPaths* 
GetRegion(RegionState* regionState, Vec2i tilePos)
{
	Vec2i coord = TileCoordToRegionCoord(tilePos);
	return (RegionPaths*)HashMapKVGet(&regionState->RegionMap, &coord);
}