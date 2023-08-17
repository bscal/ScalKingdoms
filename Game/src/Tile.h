#pragma once

#include "Core.h"
#include "TileMap.h"

#include "Structures/BitArray.h"

struct TileInfo
{
	Rectangle Src;
	Rectangle Occulution;
	Flag8 DefaultTileFlags;
	int MovementCost;
};

void TileMgrInitialize(Texture2D* tileSetTexture);

u16 TileMgrRegisterTile(const TileInfo* tileInfo);

#define RegisterTile(tilesConstName, tileInfo) (tilesConstName = TileMgrRegisterTile(&(tileInfo)))

TileInfo* GetTileInfo(u16 id);
TileInfo* GetTileInfoTile(Tile tile);
Texture2D* GetTileSheet();

Rectangle CoordToRec(int x, int y);
Rectangle CoordToRec(int x, int y, int w, int h);

Tile NewTile(u16 tileId);

namespace Tiles
{
	inline u16 EMPTY;
	inline u16 STONE;
	inline u16 BLUE_STONE;
	inline u16 WOOD_DOOR;
	inline u16 FIRE_WALL;
}
