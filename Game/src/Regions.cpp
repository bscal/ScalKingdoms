#include "Regions.h"

#include "GameState.h"
#include "Components.h"
#include "TileMap.h"
#include "Tile.h"
#include "Pathfinder.h"

constexpr global_var int MAX_REGION_SEARCH = 128;
constexpr global_var int MAX_LOCAL_SEARCH = 256;

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
TileDistance(Vec2i v0, Vec2i v1)
{
	constexpr int MOVE_COST = 10;
	constexpr int DIAGONAL_COST = 14;
	int x = abs(v0.x - v1.x);
	int y = abs(v0.y - v1.y);
	int res = MOVE_COST * (x + y) + (DIAGONAL_COST - 2 * MOVE_COST) * (int)fminf((float)x, (float)y);
	return res;
}

internal int 
RegionDistance(Vec2i v0, Vec2i v1)
{
	constexpr int MOVE_COST = 10;
	constexpr int DIAGONAL_COST = 14;
	int x = abs(v0.x - v1.x);
	int y = abs(v0.y - v1.y);
	//int res = MOVE_COST * (x + y);
	int res = MOVE_COST * (x + y) + (DIAGONAL_COST - 2 * MOVE_COST) * (int)fminf((float)x, (float)y);
	return res;
}

void RegionStateInit(RegionState* regionState)
{
	regionState->Open = BHeapCreate(zpl_heap_allocator(), RegionCompareCost, MAX_REGION_SEARCH);
	HashMapInitialize(&regionState->RegionMap, sizeof(RegionPaths), MAX_REGION_SEARCH, zpl_heap_allocator());
	HashMapInitialize(&regionState->OpenMap, sizeof(RegionNode*), MAX_REGION_SEARCH, zpl_heap_allocator());
	HashSetInitialize(&regionState->ClosedSet, MAX_REGION_SEARCH, zpl_heap_allocator());
}

void 
LoadRegionPaths(RegionState* regionState, TileMap* tilemap, Chunk* chunk)
{
	SASSERT(regionState);
	SASSERT(tilemap);
	SASSERT(chunk);
	
	Vec2i chunkWorld = ChunkToTile(chunk->Coord);

	SLOG_INFO("Loading regions for %s", FMT_VEC2I(chunk->Coord));

	for (int yDiv = 0; yDiv < DIVISIONS; ++yDiv)
	{
		for (int xDiv = 0; xDiv < DIVISIONS; ++xDiv)
		{
			RegionPaths region = {};

			Vec2i pos = chunkWorld + Vec2i{ xDiv * REGION_SIZE, yDiv * REGION_SIZE };
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

			u32 hash = HashTile(region.Pos);
			HashMapSetEx(&regionState->RegionMap, hash, &region, true);
		}
	}
}

void
FindRegionPath(RegionState* regionState, TileMap* tilemap, Vec2i startTile, Vec2i endTile, HashSet* regionSet)
{
	Vec2i start;
	start.x = (int)floorf((float)startTile.x / (float)REGION_SIZE);
	start.y = (int)floorf((float)startTile.y / (float)REGION_SIZE);

	Vec2i end;
	end.x = (int)floorf((float)endTile.x / (float)REGION_SIZE);
	end.y = (int)floorf((float)endTile.y / (float)REGION_SIZE);

	BHeapClear(regionState->Open);
	HashMapClear(&regionState->OpenMap);
	HashSetClear(&regionState->ClosedSet);

	RegionNode* node = AllocNode(RegionNode);
	node->Pos = start;
	node->Parent = nullptr;
	node->GCost = 0;
	node->HCost = RegionDistance(start, end);
	node->FCost = node->GCost + node->HCost;

	BHeapPushMin(regionState->Open, node, node);
	u32 firstHash = Hash(node->Pos);
	HashMapSet(&regionState->OpenMap, firstHash, &node->FCost);

	while (regionState->Open->Count > 0)
	{
		BHeapItem item = BHeapPopMin(regionState->Open);

		RegionNode* node = (RegionNode*)item.User;

		u32 hash = Hash(node->Pos);
		HashMapRemove(&regionState->OpenMap, hash);
		HashSetSet(&regionState->ClosedSet, hash);

		if (node->Pos == end)
		{
			if (Client.DebugShowRegionPaths)
			{
				SLOG_DEBUG("[ Debug::Pathfinding ] %d region path length, %d total regions", regionSet->Count, regionState->ClosedSet.Count);
				ArrayListClear(Client.DebugRegionsPath);
			}

			HashSetClear(regionSet);

			RegionNode* prev = node;
			while (prev)
			{
				u32 prevHash = HashTile(prev->Pos);
				HashSetSet(regionSet, prevHash);
				ArrayListPush(zpl_heap_allocator(), Client.DebugRegionsPath, prev->Pos);
				prev = prev->Parent;
			}



			return;
		}
		else
		{
			for (int i = 0; i < ArrayLength(Vec2i_NEIGHTBORS); ++i)
			{
				if (regionState->Open->Count >= MAX_LOCAL_SEARCH)
				{
					SASSERT(false);
					SLOG_DEBUG("Could not find path");
					return;
				}

				Vec2i next = node->Pos + Vec2i_NEIGHTBORS[i];

				u32 nextHash = Hash(next);
				if (HashSetContains(&regionState->ClosedSet, nextHash))
					continue;

				RegionPaths* nextRegion = HashMapGet(&regionState->RegionMap, nextHash, RegionPaths);
				if (!nextRegion || !nextRegion->Sides[i])
					continue;
				else
				{
					int* nextCost = HashMapGet(&regionState->OpenMap, nextHash, int);
					int cost = node->GCost + RegionDistance(node->Pos, next);
					if (!nextCost || cost < *nextCost)
					{
						RegionNode* nextNode = AllocNode(RegionNode);
						nextNode->Pos = next;
						nextNode->Parent = node;
						nextNode->GCost = cost;
						nextNode->HCost = RegionDistance(end, next);
						nextNode->FCost = nextNode->GCost + nextNode->HCost;

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

void 
RegionFindLocalPath(RegionState* regionState, Vec2i start, Vec2i end, CMove* moveComponent)
{
	Pathfinder* pathfinder = &GetGameState()->Pathfinder;
	TileMap* tilemap = &GetGameState()->TileMap;

	BHeapClear(pathfinder->Open);
	HashMapClear(&pathfinder->OpenSet);
	HashSetClear(&pathfinder->ClosedSet);

	Node* node = AllocNode(Node);
	node->Pos = start;
	node->Parent = nullptr;
	node->GCost = 0;
	node->HCost = TileDistance(start, end);
	node->FCost = node->GCost + node->HCost;

	BHeapPushMin(pathfinder->Open, node, node);
	u32 firstHash = Hash(node->Pos);
	HashMapSet(&pathfinder->OpenSet, firstHash, &node->FCost);

	while (pathfinder->Open->Count > 0)
	{
		BHeapItem item = BHeapPopMin(pathfinder->Open);

		Node* node = (Node*)item.User;

		u32 hash = Hash(node->Pos);
		HashMapRemove(&pathfinder->OpenSet, hash);
		HashSetSet(&pathfinder->ClosedSet, hash);

		if (node->Pos == end)
		{
			zpl_array_clear(moveComponent->TilePath);
			if (node)
			{
				int count = 0;

				zpl_array_append(moveComponent->TilePath, node->Pos);
				++count;

				Node* prev = node->Parent;
				while (prev)
				{
					zpl_array_append(moveComponent->TilePath, prev->Pos);
					prev = prev->Parent;
					++count;
				}
				SLOG_DEBUG("[ Debug::Pathfinding ] %d path length, %d total tiles", count, GetGameState()->Pathfinder.ClosedSet.Count);
			}
		}
		else
		{
			for (int i = 0; i < ArrayLength(Vec2i_NEIGHTBORS_CORNERS); ++i)
			{
				Vec2i next = node->Pos + Vec2i_NEIGHTBORS_CORNERS[i];

				RegionPaths* nextRegion = GetRegion(regionState, next);
				u32 nextRegionhash = Hash(nextRegion->Pos);
				if (!HashSetContains(&moveComponent->Regions, nextRegionhash))
					continue;

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
					int cost = node->GCost + TileDistance(node->Pos, next) + tileCost;
					if (!nextCost || cost < *nextCost)
					{
						Node* nextNode = AllocNode(Node);
						nextNode->Pos = next;
						nextNode->Parent = node;
						nextNode->GCost = cost;
						nextNode->HCost = TileDistance(end, next);
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
}

RegionPaths* 
GetRegion(RegionState* regionState, Vec2i tilePos)
{
	int regionX = (int)floorf((float)tilePos.x / (float)REGION_SIZE);
	int regionY = (int)floorf((float)tilePos.y / (float)REGION_SIZE);
	Vec2i coord = Vec2i{ regionX, regionY };
	return HashMapGet(&regionState->RegionMap, Hash(coord), RegionPaths);
}


