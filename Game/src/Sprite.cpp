#include "Sprite.h"

#define STB_RECT_PACK_IMPLEMENTATION
#include "stb/stb_rect_pack.h"

struct SpriteMgr
{
	Texture2D SpriteSheet;
	zpl_array(Sprite) Sprites;
};

global_var SpriteMgr SpriteManager;

void SpriteMgrInitialize(Texture2D spriteSheetTexture)
{
	SpriteManager.SpriteSheet = spriteSheetTexture;
}

uint16_t SpriteRegister(SpriteRect rect, Vector2 origin)
{
	if (!SpriteManager.Sprites)
		zpl_array_init_reserve(SpriteManager.Sprites, SPersistent, 64);

	zpl_isize id = zpl_array_count(SpriteManager.Sprites);
	
	if (id > UINT16_MAX)
	{
		SERR("Registering to many sprites!");
		return 0;
	}

	Sprite sprite;
	sprite.Rect = rect;
	sprite.Origin = origin;
	zpl_array_append(SpriteManager.Sprites, sprite);

	return (uint16_t)id;
}

Sprite* 
SpriteGet(uint16_t id)
{
	return &SpriteManager.Sprites[id];
}

Texture2D*
SpriteGetTexture()
{
	return &SpriteManager.SpriteSheet;
}