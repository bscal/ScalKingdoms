#include "Regions.h"

#include "GameState.h"
#include "Components.h"
#include "Pathfinder.h"
#include "TileMap.h"
#include "Tile.h"

constant_var u8 INVERSE_DIRECTIONS[] = { 2, 3, 0, 1 };

internal_var HashMapT<Vec2i, Region> RegionMap;

internal void
Pathfind(Pathfinder* pathfinder, TileMap* tilemap, Vec2i start, Vec2i end,
		 void(*callback)(Node*, void*), void* stack);

Region*
GetRegion(Vec2i tilePos)
{
	return HashMapTGet(&RegionMap, &tilePos);
}

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
	constexpr int DIAGONAL_COST = 14;
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
	constexpr size_t PATHFINDER_REGION_SIZE = 1024;
	constexpr size_t REGION_MAP_SIZE = VIEW_DISTANCE_TOTAL_CHUNKS * DIVISIONS * DIVISIONS;
	HashMapTInitialize(&RegionMap, REGION_MAP_SIZE, SAllocatorArena(&GetGameState()->GameArena));
	pathfinder->Open = BHeapCreate(SAllocatorArena(&GetGameState()->GameArena), RegionCompareCost, PATHFINDER_REGION_SIZE);
	HashMapTInitialize(&pathfinder->OpenSet, PATHFINDER_REGION_SIZE, SAllocatorArena(&GetGameState()->GameArena));
	HashSetTInitialize(&pathfinder->ClosedSet, PATHFINDER_REGION_SIZE, SAllocatorArena(&GetGameState()->GameArena));
}

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

	// Creates regions and checks each side if able to move to neighboring region
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
				Vec2i neighborTilePos = tilePos + Vec2i_CARDINALS[SIDE];

				Tile* tile = GetTile(tilemap, tilePos);
				if (!tile || tile->Flags.Get(TILE_FLAG_COLLISION))
					continue;

				Tile* neighborTile = GetTile(tilemap, neighborTilePos);
				if (!neighborTile || neighborTile->Flags.Get(TILE_FLAG_COLLISION))
					continue;

				region.Sides[SIDE] = tilePos;
				region.SideConnections[SIDE] = neighborTilePos;
				break;
			}

			for (int i = 0; i < (int)ArrayLength(REGION_SIDE_POINTS); ++i)
			{
				constexpr int SIDE = 1;

				int sideIdx = REGION_SIDE_POINTS[i];
				Vec2i offset = { REGION_SIZE - 1, sideIdx };
				Vec2i tilePos = pos + offset;
				Vec2i neighborTilePos = tilePos + Vec2i_CARDINALS[SIDE];

				Tile* tile = GetTile(tilemap, tilePos);
				if (!tile || tile->Flags.Get(TILE_FLAG_COLLISION))
					continue;

				Tile* neighborTile = GetTile(tilemap, neighborTilePos);
				if (!neighborTile || neighborTile->Flags.Get(TILE_FLAG_COLLISION))
					continue;

				region.Sides[SIDE] = tilePos;
				region.SideConnections[SIDE] = neighborTilePos;
				break;
			}

			for (int i = 0; i < (int)ArrayLength(REGION_SIDE_POINTS); ++i)
			{
				constexpr int SIDE = 2;

				int sideIdx = REGION_SIDE_POINTS[i];
				Vec2i offset = { sideIdx, REGION_SIZE - 1 };
				Vec2i tilePos = pos + offset;
				Vec2i neighborTilePos = tilePos + Vec2i_CARDINALS[SIDE];

				Tile* tile = GetTile(tilemap, tilePos);
				if (!tile || tile->Flags.Get(TILE_FLAG_COLLISION))
					continue;

				Tile* neighborTile = GetTile(tilemap, neighborTilePos);
				if (!neighborTile || neighborTile->Flags.Get(TILE_FLAG_COLLISION))
					continue;

				region.Sides[SIDE] = tilePos;
				region.SideConnections[SIDE] = neighborTilePos;
				break;
			}

			for (int i = 0; i < (int)ArrayLength(REGION_SIDE_POINTS); ++i)
			{
				constexpr int SIDE = 3;

				int sideIdx = REGION_SIDE_POINTS[i];
				Vec2i offset = { 0, sideIdx };
				Vec2i tilePos = pos + offset;
				Vec2i neighborTilePos = tilePos + Vec2i_CARDINALS[SIDE];

				Tile* tile = GetTile(tilemap, tilePos);
				if (!tile || tile->Flags.Get(TILE_FLAG_COLLISION))
					continue;

				Tile* neighborTile = GetTile(tilemap, neighborTilePos);
				if (!neighborTile || neighborTile->Flags.Get(TILE_FLAG_COLLISION))
					continue;

				region.Sides[SIDE] = tilePos;
				region.SideConnections[SIDE] = neighborTilePos;
				break;
			}

			HashMapTReplace(&RegionMap, &region.Coord, &region);
		}
	}

	// Generate paths for regions
	for (int yDiv = 0; yDiv < DIVISIONS; ++yDiv)
	{
		for (int xDiv = 0; xDiv < DIVISIONS; ++xDiv)
		{
			Vec2i regionCoord = startRegion + Vec2i{ xDiv, yDiv };
			Region* region = HashMapTGet(&RegionMap, &regionCoord);
			SAssert(region);

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
				};

				RegionPathStack stack = {};
				stack.Region = region;
				stack.Index = i;

				Pathfind(&GetGameState()->Pathfinder, &GetGameState()->TileMap, start, end,
						 [](Node* node, void* stack)
						 {
							 RegionPathStack* pathStack = (RegionPathStack*)stack;
							 u8 pathLength = pathStack->Region->PathLengths[pathStack->Index];
							 pathStack->Region->PathPaths[pathStack->Index][pathLength] = node->Pos;
							 ++pathStack->Region->PathLengths[pathStack->Index];
							 pathStack->Region->PathCost[pathStack->Index] += (short)node->GCost;
						 }, &stack);
			}
		}
	}
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
			bool removed = HashMapTRemove(&RegionMap, &pos);
			SAssert(removed);
		}
	}
}

internal void
Pathfind(Pathfinder* pathfinder, TileMap* tilemap, Vec2i start, Vec2i end, void(*callback)(Node*, void*), void* stack)
{
	BHeapClear(pathfinder->Open);
	HashMapTClear(&pathfinder->OpenSet);
	HashSetTClear(&pathfinder->ClosedSet);

	Node* node = ArenaPushStruct(&TransientState.TransientArena, Node);
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
			for (size_t i = 0; i < ArrayLength(Vec2i_NEIGHTBORS); ++i)
			{
				Vec2i nextTile = curNode->Pos + Vec2i_NEIGHTBORS[i];

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
						Node* nextNode = ArenaPushStruct(&TransientState.TransientArena, Node);
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

	moveData->PathProgress = 0;
	moveData->Path.Clear();
	moveData->StartPath.Clear();
	moveData->EndPath.Clear();
	moveData->StartRegionTilePos = tileStart;
	moveData->EndRegionTilePos = tileEnd;

	Vec2i regionStart = TileCoordToRegionCoord(tileStart);
	Vec2i regionEnd = TileCoordToRegionCoord(tileEnd);

	// If we are within 1 region radius just pathfind normally
	if (ManhattanDistance(regionStart, regionEnd) <= 14)
	{
		Pathfind(pathfinderForTiles, tilemap, tileStart, tileEnd,
				 [](Node* node, void* stack)
				 {
					 RegionMoveData* moveData = (RegionMoveData*)stack;
					 moveData->StartPath.Push(&node->Pos);
				 }, moveData);
		return;
	}

	RegionNode* node = ArenaPushStruct(&TransientState.TransientArena, RegionNode);
	node->Pos = regionStart;
	node->Parent = nullptr;
	node->GCost = 0;
	node->HCost = ManhattanDistance(regionStart, regionEnd);
	node->FCost = node->GCost + node->HCost;
	node->SideFrom = UINT8_MAX; // Start node doesn't come from anywhere, this is used if we reach the dest.

	BHeapPushMin(pathfinder->Open, node, node);
	HashMapTSet(&pathfinder->OpenSet, &node->Pos, &node);

	while (pathfinder->Open->Count > 0)
	{
		BHeapItem item = BHeapPopMin(pathfinder->Open);

		RegionNode* curNode = (RegionNode*)item.User;

		HashMapTRemove(&pathfinder->OpenSet, &curNode->Pos);
		HashSetTSet(&pathfinder->ClosedSet, &curNode->Pos);
		Region* curRegion = HashMapTGet(&RegionMap, &curNode->Pos);

		// Dest reached
		if (curNode->Pos == regionEnd)
		{
			Vec2i lastPos = curNode->Pos;
			RegionNode* prev = curNode->Parent;
			while (prev)
			{
				// Don't process start region, we start in it and will pathfind to start of
				// next region.
				if (prev->SideFrom == UINT8_MAX)
					break;

				RegionPath regionPath = {};
				regionPath.RegionCoord = prev->Pos;

				// Get's the diretions we going to.
				// Remember this is reverse order.
				// lastPos would be dest on 1st iteration
				int toDir;
				Vec2i diff = lastPos - prev->Pos;
				if (diff.x == 1)
					toDir = (int)Direction::EAST;
				else if (diff.x == -1)
					toDir = (int)Direction::WEST;
				else if (diff.y == 1)
					toDir = (int)Direction::SOUTH;
				else
					toDir = (int)Direction::NORTH;

				// Converts dirTo and sideFrom to path index
				regionPath.Direction = (RegionDirection)DIRECTION_2_REGION_DIR[prev->SideFrom][toDir];

				moveData->Path.Push(&regionPath);

				lastPos = prev->Pos;
				prev = prev->Parent;
			}
			
			// If we moved more then 2 regions away
			if (moveData->Path.Count > 0)
			{
				// cur tile -> 1st path pos (doesn't include start region)
				RegionPath firstRegionPath = *moveData->Path.Last();
				Region* firstRegion = GetRegion(firstRegionPath.RegionCoord);
				SAssert(firstRegion);
				u8 firstRegionPathLength = firstRegion->PathLengths[(int)firstRegionPath.Direction];
				Vec2i firstRegionPathTarget = firstRegion->PathPaths[(int)firstRegionPath.Direction][firstRegionPathLength - 1];
				
				// Need to pass EndPos so we can skip it when moving, it would cause a delay when moving since it is
				// the same value as the start of the region paths.
				struct PathfindToStartStack
				{
					RegionMoveData* MoveData;
					Vec2i EndPos;
				};
				PathfindToStartStack stack = PathfindToStartStack{ moveData, firstRegionPathTarget };
				Pathfind(pathfinderForTiles, tilemap, tileStart, firstRegionPathTarget,
						 [](Node* node, void* stack)
						 {
							 PathfindToStartStack* stackCasted = (PathfindToStartStack*)stack;
							 if (node->Pos != stackCasted->EndPos)
							 {
								 stackCasted->MoveData->StartPath.Push(&node->Pos);
							 }
						 }, &stack);

				// last pos in last path -> end tile
				RegionPath lastRegionPath = moveData->Path.Data[0];
				Region* lastRegion = GetRegion(lastRegionPath.RegionCoord);
				SAssert(lastRegion);
				Vec2i lastRegionPathTarget = lastRegion->PathPaths[(int)lastRegionPath.Direction][0];
				Pathfind(pathfinderForTiles, tilemap, lastRegionPathTarget, tileEnd,
						 [](Node* node, void* stack)
						 {
							 RegionMoveData* moveData = (RegionMoveData*)stack;
							 moveData->EndPath.Push(&node->Pos);
						 }, moveData);
				// We deincreament by 1 because first element is the same position as last element
				// of the region path
				--moveData->EndPath.Count;
			}
			return;
		}
		else
		{
			for (u8 neighborDirection = 0; neighborDirection < ArrayLength(Vec2i_CARDINALS); ++neighborDirection)
			{
				Vec2i regionNextCoord = curNode->Pos + Vec2i_CARDINALS[neighborDirection];

				if (curRegion->Sides[neighborDirection] == Vec2i_NULL)
					continue;

				if (HashSetTContains(&pathfinder->ClosedSet, &regionNextCoord))
					continue;

				Region* regionNext = HashMapTGet(&RegionMap, &regionNextCoord);
				if (!regionNext)
					continue;
				else
				{
					RegionNode** nextNodePtr = HashMapTGet(&pathfinder->OpenSet, &regionNextCoord);
					int dist = ManhattanDistance(curNode->Pos, regionNextCoord);
					int cost = curNode->GCost + dist + curRegion->PathCost[neighborDirection];
					if (!nextNodePtr || cost < (*nextNodePtr)->GCost)
					{
						SAssert(!nextNodePtr || (nextNodePtr && *nextNodePtr));

						RegionNode* nextNode = ArenaPushStruct(&TransientState.TransientArena, RegionNode);
						nextNode->Pos = regionNextCoord;
						nextNode->Parent = curNode;
						nextNode->GCost = cost;
						nextNode->HCost = ManhattanDistance(regionEnd, regionNextCoord);
						nextNode->FCost = nextNode->GCost + nextNode->HCost;
						// This takes inverse because neighborDirection is direction from curNode -> nextNode
						// So when we look at a node we know what side we are on.
						nextNode->SideFrom = INVERSE_DIRECTIONS[neighborDirection];

						BHeapPushMin(pathfinder->Open, nextNode, nextNode);
						if (!nextNodePtr)
						{
							HashMapTSet(&pathfinder->OpenSet, &regionNextCoord, &nextNode);
						}
						else
						{
							*nextNodePtr = nextNode;
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
	if (!ecs_is_valid(GetGameState()->World, Client.SelectedEntity))
		return;

	const CMove* move = ecs_get(GetGameState()->World, Client.SelectedEntity, CMove);
	if (move)
	{
		for (int i = 0; i < move->MoveData.Path.Count; ++i)
		{
			DrawRectangle(
				move->MoveData.Path.Data[i].RegionCoord.x * REGION_SIZE * TILE_SIZE,
				move->MoveData.Path.Data[i].RegionCoord.y * REGION_SIZE * TILE_SIZE,
				REGION_SIZE * TILE_SIZE,
				REGION_SIZE * TILE_SIZE,
				{ 255, 0, 0, 100 });
		}
	}

#if 0
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
#endif
}
