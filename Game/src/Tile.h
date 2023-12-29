#pragma once

#include "Core.h"

#include "Structures/BitArray.h"

struct TileDef
{
	Rectangle SpriteSheetRect;
	Rectangle Occulution;
	Flag8 DefaultTileFlags;
	int MovementCost;
};

enum TileFlags : u8
{
	TILE_FLAG_COLLISION = Bit(0),
	TILE_FLAG_LIQUID = Bit(1),
	TILE_FLAG_IS_HIDDEN = Bit(2),
	TILE_FLAG_BLOCKS_LIGHT = Bit(3),
};

struct Tile
{
	u16 BackgroundId;
	u16 ForegroundId;
	Flag8 Flags;
	u8 ReachabilityLevel;
};

void TileMgrInitialize(Texture2D* tileSetTexture);

u16 TileMgrRegisterTile(const TileDef* tileInfo);

#define RegisterTile(tilesConstName, tileInfo) (tilesConstName = TileMgrRegisterTile(&(tileInfo)))

TileDef* GetTileDef(u16 id);
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
