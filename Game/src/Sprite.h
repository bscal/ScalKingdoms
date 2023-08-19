#pragma once

#include "Core.h"

struct SpriteRect
{
    uint16_t x;
    uint16_t y;
    uint16_t w;
    uint16_t h;
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

uint16_t SpriteRegister(SpriteRect rect, Vector2 origin);

Sprite* SpriteGet(uint16_t id);

namespace Sprites
{
    inline uint16_t PLAYER = SpriteRegister({ 0, 0, 16, 16 }, { 8, 8 });
}
