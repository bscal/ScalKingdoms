#include "Regions.h"

#include "GameState.h"
#include "Pathfinder.h"
#include "Components.h"
#include "TileMap.h"
#include "Tile.h"

#include "Structures/ArrayList.h"

constexpr global int MAX_REGION_SEARCH = 128;
constexpr global int MAX_LOCAL_SEARCH = 256;

#define AllocNode(T) ((T*)SMalloc(Allocator::Frame, sizeof(T)));

global HashMapT<Vec2i, Region> RegionMap;

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
	Node* a = (Node*)cur;
	Node* b = (Node*)parent;
	if (a->FCost == b->FCost)
		return (a->HCost < b->HCost) ? -1 : 0;
	else if (a->FCost < b->FCost)
		return -1;
	else
		return 0;
}

internal int
ManhattanDistance(Vec2i v0, Vec2i v1)
{
	constexpr int MOVE_COST = 10;
	constexpr int DIAGONAL_COST = 29;
	int x = abs(v0.x - v1.x);
	int y = abs(v0.y - v1.y);
	int res = MOVE_COST * (x + y) + (DIAGONAL_COST - 2 * MOVE_COST) * (int)fminf((float)x, (float)y);
	return res;
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

void
PathfinderRegionsInit(RegionPathfinder* pathfinder)
{
	HashMapTInitialize(&RegionMap, 32 * 12, Allocator::Arena);
	pathfinder->Open = BHeapCreate(Allocator::Arena, RegionCompareCost, 256);
	HashMapTInitialize(&pathfinder->OpenSet, 256, Allocator::Arena);
	HashSetTInitialize(&pathfinder->ClosedSet, 256, Allocator::Arena);
}

void 
RegionUnload(Chunk* chunk)
{
	SAssert(chunk);

	Vec2i chunkWorld = ChunkToTile(chunk->Coord);
	Vec2i startRegionCoord = TileCoordToRegionCoord(chunkWorld);
	for (int yDiv = 0; yDiv < DIVISIONS; ++yDiv)
	{
		for (int xDiv = 0; xDiv < DIVISIONS; ++xDiv)
		{
			Vec2i pos = startRegionCoord + Vec2i{ xDiv, yDiv };
			Region* region = GetRegion(pos);
			SAssert(region);

			for (int i = 0; i < ArrayLength(region->Paths); ++i)
			{
				if (region->Paths[i])
				{
					ArrayListFree(Allocator::Arena, region->Paths[i]);
				}
			}
			HashMapTRemove(&RegionMap, &pos);
		}
	}
}

Region* 
GetRegion(Vec2i tilePos)
{
	return HashMapTGet(&RegionMap, &tilePos);
}

internal void
Pathfind(Pathfinder* pathfinder, TileMap* tilemap, Vec2i start, Vec2i end,
	void(*callback)(Node*, void*), void* stack);

void 
RegionLoad(TileMap* tilemap, Chunk* chunk)
{
	SAssert(tilemap);
	SAssert(chunk);

	Vec2i chunkWorld = ChunkToTile(chunk->Coord);
	Vec2i startRegion = TileCoordToRegionCoord(chunkWorld);

	Region* alreadyExistsRegion = GetRegion(startRegion);
	if (alreadyExistsRegion)
	{
		RegionUnload(chunk);
	}

	for (int yDiv = 0; yDiv < DIVISIONS; ++yDiv)
	{
		for (int xDiv = 0; xDiv < DIVISIONS; ++xDiv)
		{
			Region region = {};

			Vec2i pos = chunkWorld + Vec2i{ xDiv * REGION_SIZE, yDiv * REGION_SIZE };
			
			region.Coord = startRegion + Vec2i{ xDiv, yDiv };

			for (int i = 0; i < (int)ArrayLength(region.Sides); ++i)
			{
				region.Sides[i] = Vec2i_NULL;
			}

			for (int i = 0; i < (int)ArrayLength(REGION_SIDE_POINTS); ++i)
			{
				constexpr int SIDE = 0;

				int sideIdx = REGION_SIDE_POINTS[i];
				Vec2i offset = { sideIdx, 0 };
				Vec2i tilePos = pos + offset;
				Vec2i neighborTilePos = tilePos + Vec2i_NEIGHTBORS[SIDE];

				Tile* tile = GetTile(tilemap, tilePos);
				if (!tile || tile->Flags.Get(TILE_FLAG_COLLISION))
					continue;

				Tile* neighborTile = GetTile(tilemap, neighborTilePos);
				if (!neighborTile || neighborTile->Flags.Get(TILE_FLAG_COLLISION))
					continue;

				region.Sides[SIDE] = tilePos;
				region.Sides[SIDE] = neighborTilePos;
				break;
			}

			for (int i = 0; i < (int)ArrayLength(REGION_SIDE_POINTS); ++i)
			{
				constexpr int SIDE = 1;

				int sideIdx = REGION_SIDE_POINTS[i];
				Vec2i offset = { REGION_SIZE - 1, sideIdx };
				Vec2i tilePos = pos + offset;
				Vec2i neighborTilePos = tilePos + Vec2i_NEIGHTBORS[SIDE];

				Tile* tile = GetTile(tilemap, tilePos);
				if (!tile || tile->Flags.Get(TILE_FLAG_COLLISION))
					continue;

				Tile* neighborTile = GetTile(tilemap, neighborTilePos);
				if (!neighborTile || neighborTile->Flags.Get(TILE_FLAG_COLLISION))
					continue;

				region.Sides[SIDE] = tilePos;
				region.Sides[SIDE] = neighborTilePos;
				break;
			}

			for (int i = 0; i < (int)ArrayLength(REGION_SIDE_POINTS); ++i)
			{
				constexpr int SIDE = 2;

				int sideIdx = REGION_SIDE_POINTS[i];
				Vec2i offset = { sideIdx, REGION_SIZE - 1 };
				Vec2i tilePos = pos + offset;
				Vec2i neighborTilePos = tilePos + Vec2i_NEIGHTBORS[SIDE];

				Tile* tile = GetTile(tilemap, tilePos);
				if (!tile || tile->Flags.Get(TILE_FLAG_COLLISION))
					continue;

				Tile* neighborTile = GetTile(tilemap, neighborTilePos);
				if (!neighborTile || neighborTile->Flags.Get(TILE_FLAG_COLLISION))
					continue;

				region.Sides[SIDE] = tilePos;
				region.Sides[SIDE] = neighborTilePos;
				break;
			}

			for (int i = 0; i < (int)ArrayLength(REGION_SIDE_POINTS); ++i)
			{
				constexpr int SIDE = 3;

				int sideIdx = REGION_SIDE_POINTS[i];
				Vec2i offset = { 0, sideIdx };
				Vec2i tilePos = pos + offset;
				Vec2i neighborTilePos = tilePos + Vec2i_NEIGHTBORS[SIDE];

				Tile* tile = GetTile(tilemap, tilePos);
				if (!tile || tile->Flags.Get(TILE_FLAG_COLLISION))
					continue;

				Tile* neighborTile = GetTile(tilemap, neighborTilePos);
				if (!neighborTile || neighborTile->Flags.Get(TILE_FLAG_COLLISION))
					continue;

				region.Sides[SIDE] = tilePos;
				region.Sides[SIDE] = neighborTilePos;
				break;
			}

			HashMapTReplace(&RegionMap, &region.Coord, &region);
		}
	}
	for (int yDiv = 0; yDiv < DIVISIONS; ++yDiv)
	{
		for (int xDiv = 0; xDiv < DIVISIONS; ++xDiv)
		{
			Vec2i regionCoord = startRegion + Vec2i{ xDiv, yDiv };
			Region* region = HashMapTGet(&RegionMap, &regionCoord);
			if (!region)
				continue;

			for (int i = 0; i < REGION_DIR_MAX; ++i)
			{
				u8 nodeFrom = REGION_DIR_START[i];
				u8 nodeTo = REGION_DIR_END[i];
				SAssert(nodeFrom != nodeTo);

				Vec2i start = region->Sides[nodeFrom];
				Vec2i end = region->Sides[nodeTo];

				if (start == Vec2i_NULL || end == Vec2i_NULL)
					continue;

				struct RegionPathStack
				{
					Region* Region;
					int Index;
					int TotalCost;
				};

				RegionPathStack stack = {};
				stack.Region = region;
				stack.Index = i;

				Pathfind(&GetGameState()->Pathfinder, &GetGameState()->TileMap, start, end,
					[](Node* node, void* stack)
					{
						RegionPathStack* pathStack = (RegionPathStack*)stack;
						ArrayListPush(Allocator::Arena, pathStack->Region->Paths[pathStack->Index], node->Pos);
						pathStack->Region->PathCost[pathStack->Index] += node->GCost;
					}, &stack);

				ArrayListPush(Allocator::Arena, region->Paths[i], region->SideConnections[nodeTo]);
			}
		}
	}
}

internal void 
Pathfind(Pathfinder* pathfinder, TileMap* tilemap, Vec2i start, Vec2i end, void(*callback)(Node*, void*), void* stack)
{
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
	HashMapTSet(&pathfinder->OpenSet, &node->Pos, &node->GCost);

	while (pathfinder->Open->Count > 0)
	{
		BHeapItem item = BHeapPopMin(pathfinder->Open);

		Node* curNode = (Node*)item.User;

		HashMapTRemove(&pathfinder->OpenSet, &curNode->Pos);
		HashSetTSet(&pathfinder->ClosedSet, &curNode->Pos);

		if (curNode->Pos == end)
		{
			Node* prev = curNode;
			while (prev)
			{
				callback(prev, stack);
				prev = prev->Parent;
			}
			return;
		}
		else
		{
			for (size_t i = 0; i < ArrayLength(Vec2i_NEIGHTBORS_CORNERS); ++i)
			{
				Vec2i nextTile = curNode->Pos + Vec2i_NEIGHTBORS_CORNERS[i];

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
						nextNode->HCost = CalculateDistance(nextTile, end);
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
}

void
PathfindRegion(Vec2i tileStart, Vec2i tileEnd, RegionMoveData* moveData)
{
	RegionPathfinder* pathfinder = &GetGameState()->RegionPathfinder;
	Pathfinder* pathfinderForTiles = &GetGameState()->Pathfinder;
	TileMap* tilemap = &GetGameState()->TileMap;

	BHeapClear(pathfinder->Open);
	HashMapTClear(&pathfinder->OpenSet);
	HashSetTClear(&pathfinder->ClosedSet);

	moveData->NeedToPathfindToStart = false;
	moveData->NeedToPathfindToEnd = false;
	moveData->StartRegionTilePos = tileStart;
	moveData->EndRegionTilePos = tileEnd;
	ArrayListClear(moveData->StartPath);
	ArrayListClear(moveData->EndPath);
	ArrayListClear(moveData->RegionPath);


	Vec2i regionStart = TileCoordToRegionCoord(tileStart);
	Vec2i regionEnd = TileCoordToRegionCoord(tileEnd);

	Region* startingRegion = HashMapTGet(&RegionMap, &regionStart);
	SAssert(startingRegion);

	Vec2i firstTarget = Vec2i_NULL;
	int shortestDist = INT32_MAX;
	u8 shortestDir = 0;
	for (u8 i = 0; i < 4; ++i)
	{
		if (startingRegion->Sides[i] != Vec2i_NULL)
		{
			firstTarget = startingRegion->Sides[i];
			int dist = CalculateDistance(firstTarget, tileEnd);
			if (dist < shortestDist)
			{
				shortestDist = dist;
				shortestDir = i;
			}
		}
	}
	SAssert(firstTarget != Vec2i_NULL);

	moveData->NeedToPathfindToStart = true;
	Pathfind(pathfinderForTiles, tilemap, tileStart, firstTarget,
		[](Node* node, void* stack)
		{
			RegionMoveData* moveData = (RegionMoveData*)stack;
			ArrayListPush(Allocator::Arena, moveData->StartPath, node->Pos);
		}, moveData);

	RegionNode* node = AllocNode(RegionNode);
	node->Pos = regionStart + Vec2i_NEIGHTBORS[shortestDir];
	node->Parent = nullptr;
	node->GCost = 0;
	node->HCost = ManhattanDistance(tileStart, tileEnd);
	node->FCost = node->GCost + node->HCost;
	node->SideFrom = shortestDir;
	node->SideTo = 0;

	BHeapPushMin(pathfinder->Open, node, node);
	HashMapTSet(&pathfinder->OpenSet, &node->Pos, &node);

	while (pathfinder->Open->Count > 0)
	{
		BHeapItem item = BHeapPopMin(pathfinder->Open);

		RegionNode* curNode = (RegionNode*)item.User;

		HashMapTRemove(&pathfinder->OpenSet, &curNode->Pos);
		HashSetTSet(&pathfinder->ClosedSet, &curNode->Pos);
		Region* curRegion = HashMapTGet(&RegionMap, &curNode->Pos);

		if (curNode->Pos == regionEnd)
		{
			Vec2i curTile = curRegion->Sides[curNode->SideFrom];

			if (curTile != tileEnd)
			{
				moveData->NeedToPathfindToEnd = true;
				Pathfind(pathfinderForTiles, tilemap, curTile, tileEnd,
					[](Node* node, void* stack)
					{
						RegionMoveData* moveData = (RegionMoveData*)stack;
						ArrayListPush(Allocator::Arena, moveData->EndPath, node->Pos);
					}, moveData);
			}

			RegionNode* prev = curNode;
			while (prev)
			{
				RegionPath regionPath = {};
				regionPath.RegionCoord = prev->Pos;
				regionPath.Direction = (RegionDirection)DIRECTION_2_REGION_DIR[prev->SideFrom][prev->SideTo];
				ArrayListPush(Allocator::Arena, moveData->RegionPath, regionPath);
				prev = prev->Parent;
			}
			return;
		}
		else
		{
			constexpr u8 INVERSE[] = { 2, 3, 0, 1 };
			for (u8 neighborDirection = 0; neighborDirection < ArrayLength(Vec2i_NEIGHTBORS); ++neighborDirection)
			{
				Vec2i regionNextCoord = curNode->Pos + Vec2i_NEIGHTBORS[neighborDirection];

				if (HashSetTContains(&pathfinder->ClosedSet, &regionNextCoord))
					continue;

				if (curRegion->Sides[neighborDirection] == Vec2i_NULL)
					continue;


				Region* regionNext = HashMapTGet(&RegionMap, &regionNextCoord);
				if (!regionNext)
					continue;
				else
				{
					//u8 pathDir = DIRECTION_2_REGION_DIR[curNode->SideFrom][neighborDirection];
					RegionNode** nextNodePtr = HashMapTGet(&pathfinder->OpenSet, &regionNextCoord);
					int dist = ManhattanDistance(curRegion->Sides[curNode->SideFrom], regionNext->Sides[INVERSE[neighborDirection]]);
					int cost = curNode->GCost + dist;
					if (!nextNodePtr || cost < (*nextNodePtr)->GCost)
					{
						SAssert(!nextNodePtr || (nextNodePtr && *nextNodePtr));
						RegionNode* nextNode = AllocNode(RegionNode);


						nextNode->Pos = regionNextCoord;
						nextNode->Parent = curNode;
						nextNode->GCost = cost;
						nextNode->HCost = ManhattanDistance(regionNext->Sides[INVERSE[neighborDirection]], tileEnd);
						nextNode->FCost = nextNode->GCost + nextNode->HCost;
						nextNode->SideFrom = INVERSE[neighborDirection];
						curNode->SideTo = neighborDirection;

						BHeapPushMin(pathfinder->Open, nextNode, nextNode);
						if (!nextNodePtr)
						{
							HashMapTSet(&pathfinder->OpenSet, &regionNextCoord, &nextNode);
						}
					}
				}
			}
		}
	}
}

void 
DrawRegions()
{
	for (int i = 0; i < ArrayListCount(Client.MoveData.RegionPath); ++i)
	{
		DrawRectangle(
			Client.MoveData.RegionPath[i].RegionCoord.x * REGION_SIZE * TILE_SIZE,
			Client.MoveData.RegionPath[i].RegionCoord.y * REGION_SIZE * TILE_SIZE,
			REGION_SIZE * TILE_SIZE,
			REGION_SIZE * TILE_SIZE,
			RED);
	}

	for (u32 i = 0; i < RegionMap.Capacity; ++i)
	{
		if (RegionMap.Buckets[i].IsUsed)
		{
			for (int side = 0; side < (int)ArrayLength(RegionMap.Buckets[i].Value.Sides); ++side)
			{
				Region* region = &RegionMap.Buckets[i].Value;
				Vec2i pos = region->Sides[side];
				DrawRectangleLines(region->Coord.x * REGION_SIZE * TILE_SIZE, region->Coord.y * REGION_SIZE * TILE_SIZE, REGION_SIZE * TILE_SIZE, REGION_SIZE * TILE_SIZE, RED);
				if (pos != Vec2i_NULL)
					DrawRectangle(pos.x * TILE_SIZE, pos.y * TILE_SIZE, 4, 4, RED);

				//for (int dir = 0; dir < REGION_DIR_MAX; ++dir)
				//{
				//	for (int idx = 0; idx < (int)ArrayListCount(RegionMap.Buckets[i].Value.Paths[dir]); ++idx)
				//	{
				//		Vec2i tile = RegionMap.Buckets[i].Value.Paths[dir][idx];
				//		DrawRectangle(tile.x * TILE_SIZE, tile.y * TILE_SIZE, TILE_SIZE, TILE_SIZE, BLUE);
				//	}
				//}
			}
		}
	}
}
