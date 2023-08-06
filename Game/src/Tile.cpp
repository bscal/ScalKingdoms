#include "Tile.h"

struct TileMgr
{
	zpl_array(TileInfo) TileDefinitions;
	Texture2D* TileSetTexture;
};

global_var TileMgr TileManager;

void TileMgrInitialize(Texture2D* tileSetTexture)
{
	TileManager.TileSetTexture = tileSetTexture;

	zpl_array_init_reserve(TileManager.TileDefinitions, zpl_heap_allocator(), 64);

	TileInfo empty = {};
	empty.Src = CoordToRec(1, 0);
	RegisterTile(Tiles::EMPTY, empty);

	TileInfo stone = {};
	stone.Src = CoordToRec(4, 0);
	Tiles::STONE = TileMgrRegisterTile(&stone);

	TileInfo blueStone = {};
	blueStone.Src = CoordToRec(1, 3);
	Tiles::BLUE_STONE = TileMgrRegisterTile(&blueStone);

	TileInfo woodDoor = {};
	woodDoor.Src = CoordToRec(3, 2);
	RegisterTile(Tiles::WOOD_DOOR, woodDoor);
}

uint16_t TileMgrRegisterTile(const TileInfo* tileInfo)
{
	uint16_t id = (uint16_t)zpl_array_count(TileManager.TileDefinitions);

	zpl_array_append(TileManager.TileDefinitions, *tileInfo);
	
	return id;
}

TileInfo* GetTileInfo(uint16_t id)
{
	SASSERT(TileManager.TileDefinitions);
	SASSERT(id < zpl_array_count(TileManager.TileDefinitions));
	if (id >= zpl_array_count(TileManager.TileDefinitions))
		return &TileManager.TileDefinitions[0];

	return &TileManager.TileDefinitions[id];
}

Texture2D* GetTileSheet()
{
	SASSERT(TileManager.TileSetTexture->id > 0);
	return TileManager.TileSetTexture;
}

Rectangle CoordToRec(int x, int y)
{
	return CoordToRec(x, y, TILE_SIZE, TILE_SIZE);
}

Rectangle CoordToRec(int x, int y, int w, int h)
{
	Rectangle r;
	r.x = (float)x * TILE_SIZE;
	r.y = (float)y * TILE_SIZE;
	r.width = (float)w;
	r.height = (float)h;
	return r;
}