#pragma once

#include "Core.h"

#include "GameState.h"
#include "RenderUtils.h"

#include "Structures/BitArray.h"
#include "Structures/StaticArray.h"

constant_var int LIGHT_MAX_RADIUS = 32;
constant_var int LIGHTMAP_WIDTH = 64;
constant_var int LIGHTMAP_WIDTH_HALF = LIGHTMAP_WIDTH / 2;

// Used to translate tile coordinates 
constant_var i8 TranslationTable[8][4] =
{
	{  1,  0,  0, -1 },
	{  0,  1, -1,  0 },
	{  0, -1, -1,  0 },
	{ -1,  0,  0, -1 },
	{ -1,  0,  0,  1 },
	{  0, -1,  1,  0 },
	{  0,  1,  1,  0 },
	{  1,  0,  0,  1 },
};

struct LightMapColor
{
	float r;
	float g;
	float b;
	float a;
};

struct Light
{
	LightMapColor Color;
	Vec2i Pos;
	u8 Radius;
};

struct LightMap
{
	RenderTexture2D Texture;
	Vec2i Position;
	Vec2i Dimensions;
	BitArray<LIGHTMAP_WIDTH * LIGHTMAP_WIDTH> WasTileUpdatedSet;
	StaticArray<LightMapColor, LIGHTMAP_WIDTH * LIGHTMAP_WIDTH> Colors;
};

global_var LightMap LightMapState;

inline void
LightMapInitialize()
{
	Vec2i reso = { LIGHTMAP_WIDTH, LIGHTMAP_WIDTH };
	LightMapState.Texture = LoadRenderTextureEx(reso, PIXELFORMAT_UNCOMPRESSED_R32G32B32A32, false);
	SInfoLog("Initialized LightMap");
	SAssert(LightMapState.Texture.id);
}
void
UpdateLight(Light* light);
inline void
LightMapUpdate(GameState* gameState)
{
	UpdateTexture(LightMapState.Texture.texture, LightMapState.Colors.Data);
	SZero(LightMapState.Colors.Data, LightMapState.Colors.MemorySize());

	Light light;
	light.Color = { 1.f, 1.f, 1.f, .1f };
	light.Pos = { 0,0 };
	light.Radius = 5;
	UpdateLight(&light);

	//const CTransform* transform = ecs_get(State.World, Client.Player, CTransform);
	LightMapState.Position.x = (int)floorf(State.Camera.target.x / TILE_SIZE) - LIGHTMAP_WIDTH_HALF;
	LightMapState.Position.y = (int)floorf(State.Camera.target.y / TILE_SIZE) - LIGHTMAP_WIDTH_HALF;
	LightMapState.Dimensions.x = LightMapState.Position.x + LIGHTMAP_WIDTH;
	LightMapState.Dimensions.y = LightMapState.Position.y + LIGHTMAP_WIDTH;
}

inline bool
IsTileInLightMap(Vec2i tile)
{
	return tile.x >= LightMapState.Position.x
		&& tile.y >= LightMapState.Position.y
		&& tile.x < LightMapState.Dimensions.x
		&& tile.y < LightMapState.Dimensions.y;
}

inline Vec2i
WorldToLightMap(Vec2i tile)
{
	Vec2i res;
	res.x = tile.x - LightMapState.Position.x;
	res.y = tile.y - LightMapState.Position.y;
	return res;
}

struct Slope
{
	int y;
	int x;

	_FORCE_INLINE_ bool Greater(int otherY, int otherX) { return this->y * otherX > this->x * otherY; } // this > y/x
	_FORCE_INLINE_ bool GreaterOrEqual(int otherY, int otherX) { return this->y * otherX >= this->x * otherY; } // this >= y/x
	_FORCE_INLINE_ bool Less(int otherY, int otherX) { return this->y * otherX < this->x * otherY; } // this < y/x
};

_FORCE_INLINE_ bool 
DoesBlockLight(Vec2i coord)
{
	Tile* tile = GetTile(&State.TileMap, coord);
	return tile && tile->Flags.Get(TILE_FLAG_BLOCKS_LIGHT);
}

internal void
ProcessOctants(Light* light, int radius, u8 octant, int x, Slope top, Slope bottom)
{
	for (; x <= radius; ++x) // rangeLimit < 0 || x <= rangeLimit
	{
		// compute the Y coordinates where the top vector leaves the column (on the right) and where the bottom vector
		// enters the column (on the left). this equals (x+0.5)*top+0.5 and (x-0.5)*bottom+0.5 respectively, which can
		// be computed like (x+0.5)*top+0.5 = (2(x+0.5)*top+1)/2 = ((2x+1)*top+1)/2 to avoid floating point math
		int topY = top.x == 1 ? x : ((x * 2 + 1) * top.y + top.x - 1) / (top.x * 2); // the rounding is a bit tricky, though
		int bottomY = bottom.y == 0 ? 0 : ((x * 2 - 1) * bottom.y + bottom.x) / (bottom.x * 2);

		int wasOpaque = -1; // 0:false, 1:true, -1:not applicable
		for (int y = topY; y >= bottomY; --y)
		{
			Vector2i txty = light->Pos;
			txty.x += x * TranslationTable[octant][0] + y * TranslationTable[octant][1];
			txty.y += x * TranslationTable[octant][2] + y * TranslationTable[octant][3];

			int distance;
			bool inRange = IsTileInLightMap(txty)
				&& ((distance = Vector2i{ x, y }.ManhattanDistanceWithCosts({})) <= radius * 10);

			if (inRange)
			{
				Vec2i lightMapCoord = WorldToLightMap(txty);
				int idx = lightMapCoord.x + lightMapCoord.y * LIGHTMAP_WIDTH;
				if (!LightMapState.WasTileUpdatedSet.Get(idx))
				{
					LightMapState.WasTileUpdatedSet.Set(idx);
					LightMapState.Colors.At(idx)->r += light->Color.r;
					LightMapState.Colors.At(idx)->g += light->Color.g;
					LightMapState.Colors.At(idx)->b += light->Color.b;
					LightMapState.Colors.At(idx)->a += light->Color.a;
				}
			}

			// NOTE: use the next line instead if you want the algorithm to be symmetrical
			// if(inRange && (y != topY || top.Y*x >= top.X*y) && (y != bottomY || bottom.Y*x <= bottom.X*y)) SetVisible(tx, ty);

			bool isOpaque = !inRange || DoesBlockLight(txty);
			if (x != radius)
			{
				if (isOpaque)
				{
					if (wasOpaque == 0) // if we found a transition from clear to opaque, this sector is done in this column, so
					{                  // adjust the bottom vector upwards and continue processing it in the next column.
						Slope newBottom = { y * 2 + 1, x * 2 - 1 }; // (x*2-1, y*2+1) is a vector to the top-left of the opaque tile
						if (!inRange || y == bottomY) { bottom = newBottom; break; } // don't recurse unless we have to
						else ProcessOctants(light, radius, octant, x + 1, top, newBottom);
					}
					wasOpaque = 1;
				}
				else // adjust top vector downwards and continue if we found a transition from opaque to clear
				{    // (x*2+1, y*2+1) is the top-right corner of the clear tile (i.e. the bottom-right of the opaque tile)
					if (wasOpaque > 0) top = { y * 2 + 1, x * 2 + 1 };
					wasOpaque = 0;
				}
			}
		}
		if (wasOpaque != 0)
			break; // if the column ended in a clear tile, continue processing the current sector
	}
}

void
UpdateLight(Light* light)
{
	LightMapState.WasTileUpdatedSet.Reset();

	Vec2i lightMapCoord = WorldToLightMap(light->Pos);
	size_t idx = lightMapCoord.x + lightMapCoord.y * LIGHTMAP_WIDTH;
	LightMapState.WasTileUpdatedSet.Set(idx);
	LightMapState.Colors.At(idx)->r += light->Color.r;
	LightMapState.Colors.At(idx)->g += light->Color.g;
	LightMapState.Colors.At(idx)->b += light->Color.b;
	LightMapState.Colors.At(idx)->a += light->Color.a;

	int radius = light->Radius;
	for (u8 octant = 0; octant < 8; ++octant)
	{
		ProcessOctants(light, radius, octant, 1, { 1, 1 }, { 0, 1 });
	}
}
