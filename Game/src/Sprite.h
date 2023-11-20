#pragma once

#include "Core.h"

#include "Structures/StaticArray.h"

struct SpriteRect
{
    u16 x;
    u16 y;
    u16 w;
    u16 h;
};

struct Sprite
{
    SpriteRect Rect;
    Vector2 Origin;

    _FORCE_INLINE_ Rectangle CastRect() const
    {
        return { (float)Rect.x, (float)Rect.y, (float)Rect.w, (float)Rect.h  };
    }
};

struct SpriteMgr
{
	Buffer<Sprite, 256> Sprites;
};

global_var SpriteMgr SpriteManager;

_FORCE_INLINE_ Sprite* 
SpriteGet(u16 id)
{
	if (id >= SpriteManager.Sprites.Count)
	{
		SWarn("SpriteGet id is >= the Count, id = %u, count = %d", id, SpriteManager.Sprites.Count);
		return SpriteManager.Sprites.Data;
	}
	else
	{
		return SpriteManager.Sprites.Data + id;
	}
}

inline u16 
SpriteRegister(SpriteRect rect, Vector2 origin)
{
	SAssert(rect.w > 0 && rect.h > 0);

	u16 id = (u16)SpriteManager.Sprites.Count;

	if (id >= SpriteManager.Sprites.Capacity)
	{
		SError("Registering to many sprites!");
		return 0;
	}

	Sprite sprite;
	sprite.Rect = rect;
	sprite.Origin = origin;

	SpriteManager.Sprites.Data[id] = sprite;
	++SpriteManager.Sprites.Count;

	return id;
}

#define SpriteDefine(name, spriteRect, origin) inline u16 name = SpriteRegister(Unpack spriteRect, Unpack origin)

namespace Sprites
{
    SpriteDefine(PLAYER, ({ 0, 0, 16, 16 }), ({ 8, 8 }));
}
