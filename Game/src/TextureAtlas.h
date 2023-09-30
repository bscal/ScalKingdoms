#pragma once

#include "Core.h"
#include "Structures/ArrayList.h"
#include "Structures/HashMapStr.h"

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
	ArrayList(Rect16) Rects;
	HashMapStr NameToIndex;
};

SpriteAtlas SpriteAtlasLoad(const char* dirPath, const char* atlasFile);
void SpriteAtlasUnload(SpriteAtlas* atlas);

struct SpriteAtlasGetResult
{
	Rectangle Rect;
	bool WasFound;
};
SpriteAtlasGetResult SpriteAtlasGet(SpriteAtlas* atlas, const char* name);
