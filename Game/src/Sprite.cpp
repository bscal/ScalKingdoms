#include "Sprite.h"

#include "Memory.h"
#include "Structures/ArrayList.h"

//#define STB_RECT_PACK_IMPLEMENTATION
//#include "stb/stb_rect_pack.h"

struct SpriteMgr
{
	ArrayList(Sprite) Sprites;
};

internal_var SpriteMgr SpriteManager;

void 
SpritesInitialize()
{
	ArrayListReserve(Allocator::Arena, SpriteManager.Sprites, 64);

	Sprites::PLAYER = SpriteRegister({ 0, 0, 16, 16 }, { 8, 8 });
}

uint16_t 
SpriteRegister(SpriteRect rect, Vector2 origin)
{
	SAssertMsg(SpriteManager.Sprites, "SpritesInitialize not called");

	u64 id = ArrayListCount(SpriteManager.Sprites);
	
	if (id > UINT16_MAX)
	{
		SError("Registering to many sprites!");
		return 0;
	}

	Sprite sprite;
	sprite.Rect = rect;
	sprite.Origin = origin;
	ArrayListPush(Allocator::Arena, SpriteManager.Sprites, sprite);

	return (uint16_t)id;
}

Sprite* 
SpriteGet(uint16_t id)
{
	return &SpriteManager.Sprites[id];
}
