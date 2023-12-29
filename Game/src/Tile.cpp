#include "Tile.h"


#include "Structures/ArrayList.h"

struct TileMgr
{
	Texture2D* TileSetTexture;
	ArrayList(TileDef) TileDefinitions;
};

internal_var TileMgr TileManager;

void TileMgrInitialize(Texture2D* tileSetTexture)
{
	TileManager.TileSetTexture = tileSetTexture;

	ArrayListReserve(SAllocatorGeneral(), TileManager.TileDefinitions, 64);

	TileDef empty = {};
	empty.SpriteSheetRect = CoordToRec(1, 0);
	RegisterTile(Tiles::EMPTY, empty);

	TileDef stone = {};
	stone.SpriteSheetRect = CoordToRec(4, 0);
	Tiles::STONE = TileMgrRegisterTile(&stone);

	TileDef blueStone = {};
	blueStone.SpriteSheetRect = CoordToRec(1, 3);
	Tiles::BLUE_STONE = TileMgrRegisterTile(&blueStone);

	TileDef wall = {};
	wall.SpriteSheetRect = CoordToRec(17, 3);
	wall.DefaultTileFlags.Set(TILE_FLAG_COLLISION, true);
	Tiles::FIRE_WALL = TileMgrRegisterTile(&wall);

	TileDef woodDoor = {};
	woodDoor.SpriteSheetRect = CoordToRec(3, 2);
	RegisterTile(Tiles::WOOD_DOOR, woodDoor);
}

u16 TileMgrRegisterTile(const TileDef* tileInfo)
{
	u16 id = (uint16_t)ArrayListCount(TileManager.TileDefinitions);

	ArrayListPush(SAllocatorGeneral(), TileManager.TileDefinitions, *tileInfo);
	
	return id;
}

TileDef* GetTileDef(u16 id)
{
	SAssert(TileManager.TileDefinitions);
	SAssert(id < ArrayListCount(TileManager.TileDefinitions));

	if (id >= ArrayListCount(TileManager.TileDefinitions))
		return &TileManager.TileDefinitions[0];

	return &TileManager.TileDefinitions[id];
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
