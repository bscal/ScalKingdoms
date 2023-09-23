#pragma once

#include "Core.h"

RenderTexture2D
LoadRenderTextureEx(Vec2i resolution, PixelFormat format, bool useDepth);

void
DrawSprite(Texture2D* texture, Rectangle source, Rectangle dest, Vec2 origin, Color tint, bool flipX);

Font
LoadBMPFontFromTexture(const char* bmpFontFile, Texture2D* atlas, Vec2 offset);

void UnloadBMPFontData(Font* font);

enum RichTextTypes
{
	RICHTEXT_COLOR = 'c',
	RICHTEXT_IMG = 'i',
	RICHTEXT_TOOLTIP = 't',
};

#define RICH_TEXT_MAX_LENGTH 29

// Draws text that can handle special keywords in the text.
// Keywords and their parameters follow this format. ${0,param1,paaram2}
// Keywords are a character following the ${ . I might switch to using a string
// for them. There characters are found in RichTextTypes enum
void
DrawRichText(const Font* _RESTRICT_ font, const char* _RESTRICT_ text,
	Vector2 position, float fontSize, float spacing, Color tint);

const char*
NewRichTextColor(int color);
