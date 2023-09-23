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

void SpritesInitialize();

uint16_t SpriteRegister(SpriteRect rect, Vector2 origin);

Sprite* SpriteGet(uint16_t id);

#define SpriteDef inline u16
namespace Sprites
{
    SpriteDef PLAYER;
}
