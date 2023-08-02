#pragma once

#include "Core.h"

RenderTexture2D
LoadRenderTextureEx(Vec2i resolution, PixelFormat format, bool useDepth);

void DrawSprite(Texture2D* texture, Rectangle source, Rectangle dest, Vec2 origin, Color tint, bool flipX);