#include "Sprite.h"

#include "Memory.h"

//#define STB_RECT_PACK_IMPLEMENTATION
//#include "stb/stb_rect_pack.h"

struct SpriteMgr
{
	zpl_array(Sprite) Sprites;
};

global SpriteMgr SpriteManager;

void 
SpritesInitialize()
{
	zpl_array_init_reserve(SpriteManager.Sprites, ZplAllocatorArena, 64);

	Sprites::PLAYER = SpriteRegister({ 0, 0, 16, 16 }, { 8, 8 });
}

uint16_t 
SpriteRegister(SpriteRect rect, Vector2 origin)
{
	SAssertMsg(SpriteManager.Sprites, "SpritesInitialize not called");

	zpl_isize id = zpl_array_count(SpriteManager.Sprites);
	
	if (id > UINT16_MAX)
	{
		SError("Registering to many sprites!");
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
