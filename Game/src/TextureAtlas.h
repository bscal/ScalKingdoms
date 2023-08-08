#pragma once

#include "Core.h"
#include "Structures/HashMap.h"

struct Rect16
{
	uint16_t x;
	uint16_t y;
	uint16_t w;
	uint16_t h;
};

struct SpriteAtlas
{
	Texture2D Texture;
	zpl_array(Rect16) Rects;
	HashMap NameToIndex;
};

SpriteAtlas SpriteAtlasLoad(const char* dirPath, const char* atlasFile);
void SpriteAtlasUnload(SpriteAtlas* atlas);
Rectangle SpriteAtlasGet(SpriteAtlas* atlas, const char* name);
