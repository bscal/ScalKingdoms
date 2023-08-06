#pragma once

#include "Core.h"
#include "TileMap.h"

struct TileInfo
{
	Rectangle Src;
	Rectangle Occulution;
};

void TileMgrInitialize(Texture2D* tileSetTexture);

uint16_t TileMgrRegisterTile(const TileInfo* tileInfo);

#define RegisterTile(tilesConstName, tileInfo) (tilesConstName = TileMgrRegisterTile(&(tileInfo)))

TileInfo* GetTileInfo(uint16_t id);
Texture2D* GetTileSheet();

Rectangle CoordToRec(int x, int y);
Rectangle CoordToRec(int x, int y, int w, int h);

namespace Tiles
{
	inline uint16_t EMPTY;
	inline uint16_t STONE;
	inline uint16_t BLUE_STONE;
	inline uint16_t WOOD_DOOR;
}
