#include "Tile.h"

struct TileMgr
{
	Texture2D* TileSetTexture;
	zpl_array(TileInfo) TileDefinitions;
};

global TileMgr TileManager;

void TileMgrInitialize(Texture2D* tileSetTexture)
{
	TileManager.TileSetTexture = tileSetTexture;

	zpl_array_init_reserve(TileManager.TileDefinitions, ZplAllocatorArena, 64);

	TileInfo empty = {};
	empty.Src = CoordToRec(1, 0);
	RegisterTile(Tiles::EMPTY, empty);

	TileInfo stone = {};
	stone.Src = CoordToRec(4, 0);
	Tiles::STONE = TileMgrRegisterTile(&stone);

	TileInfo blueStone = {};
	blueStone.Src = CoordToRec(1, 3);
	Tiles::BLUE_STONE = TileMgrRegisterTile(&blueStone);

	TileInfo wall = {};
	wall.Src = CoordToRec(17, 3);
	wall.DefaultTileFlags.Set(TILE_FLAG_COLLISION, true);
	Tiles::FIRE_WALL = TileMgrRegisterTile(&wall);

	TileInfo woodDoor = {};
	woodDoor.Src = CoordToRec(3, 2);
	RegisterTile(Tiles::WOOD_DOOR, woodDoor);
}

u16 TileMgrRegisterTile(const TileInfo* tileInfo)
{
	u16 id = (uint16_t)zpl_array_count(TileManager.TileDefinitions);

	zpl_array_append(TileManager.TileDefinitions, *tileInfo);
	
	return id;
}

TileInfo* GetTileInfo(u16 id)
{
	SAssert(TileManager.TileDefinitions);
	SAssert(id < zpl_array_count(TileManager.TileDefinitions));
	if (id >= zpl_array_count(TileManager.TileDefinitions))
		return &TileManager.TileDefinitions[0];

	return &TileManager.TileDefinitions[id];
}

TileInfo* GetTileInfoTile(Tile tile)
{
	return GetTileInfo(tile.BackgroundId);
}

Texture2D* GetTileSheet()
{
	SAssert(TileManager.TileSetTexture->id > 0);
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

Tile NewTile(u16 bgId)
{
	Tile tile = {};
	tile.BackgroundId = bgId;
	return tile;
}
