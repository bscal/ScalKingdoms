#include "RenderUtils.h"

#include <raylib/src/rlgl.h>

RenderTexture2D
LoadRenderTextureEx(Vec2i resolution, PixelFormat format, bool useDepth)
{
	RenderTexture2D target = {};

	target.id = rlLoadFramebuffer(resolution.x, resolution.y);   // Load an empty framebuffer

	if (target.id > 0)
	{
		rlEnableFramebuffer(target.id);

		// Create color texture (default to RGBA)
		target.texture.id = rlLoadTexture(nullptr, resolution.x, resolution.y, format, 1);
		target.texture.width = resolution.x;
		target.texture.height = resolution.y;
		target.texture.format = format;
		target.texture.mipmaps = 1;

		if (useDepth)
		{
			// Create depth renderbuffer/texture
			target.depth.id = rlLoadTextureDepth(resolution.x, resolution.y, true);
			target.depth.width = resolution.x;
			target.depth.height = resolution.y;
			target.depth.format = 19;       //DEPTH_COMPONENT_24BIT?
			target.depth.mipmaps = 1;
		}

		// Attach color texture and depth renderbuffer/texture to FBO
		rlFramebufferAttach(target.id, target.texture.id, RL_ATTACHMENT_COLOR_CHANNEL0, RL_ATTACHMENT_TEXTURE2D, 0);

		if (useDepth)
		{
			rlFramebufferAttach(target.id, target.depth.id, RL_ATTACHMENT_DEPTH, RL_ATTACHMENT_RENDERBUFFER, 0);
		}

		// Check if fbo is complete with attachments (valid)
		if (rlFramebufferComplete(target.id))
			SLOG_INFO("FBO: [ID %i] Framebuffer object created successfully", target.id);

		rlDisableFramebuffer();
	}
	else
	{
		SERR("FBO: Framebuffer object can not be created");
	}

	return target;
}

void
DrawSprite(Texture2D* texture, Rectangle source, Rectangle dest, Vec2 origin, Color tint, bool flipX)
{
	SASSERT(texture);
	SASSERT(IsTextureReady(*texture));

    float width = (float)texture->width;
    float height = (float)texture->height;

    //if (source.height < 0) source.y -= source.height;

    Vector2 topLeft;
    Vector2 topRight;
    Vector2 bottomLeft;
    Vector2 bottomRight;

    constexpr float rotation = 0;
    // Only calculate rotation if needed
    if (rotation == 0.0f)
    {
        float x = dest.x - origin.x;
        float y = dest.y - origin.y;
        topLeft = Vector2{ x, y };
        topRight = Vector2{ x + dest.width, y };
        bottomLeft = Vector2{ x, y + dest.height };
        bottomRight = Vector2{ x + dest.width, y + dest.height };
    }
    else
    {
        float sinRotation = sinf(rotation * DEG2RAD);
        float cosRotation = cosf(rotation * DEG2RAD);
        float x = dest.x;
        float y = dest.y;
        float dx = -origin.x;
        float dy = -origin.y;

        topLeft.x = x + dx * cosRotation - dy * sinRotation;
        topLeft.y = y + dx * sinRotation + dy * cosRotation;

        topRight.x = x + (dx + dest.width) * cosRotation - dy * sinRotation;
        topRight.y = y + (dx + dest.width) * sinRotation + dy * cosRotation;

        bottomLeft.x = x + dx * cosRotation - (dy + dest.height) * sinRotation;
        bottomLeft.y = y + dx * sinRotation + (dy + dest.height) * cosRotation;

        bottomRight.x = x + (dx + dest.width) * cosRotation - (dy + dest.height) * sinRotation;
        bottomRight.y = y + (dx + dest.width) * sinRotation + (dy + dest.height) * cosRotation;
    }

    rlSetTexture(texture->id);
    rlBegin(RL_QUADS);

    rlColor4ub(tint.r, tint.g, tint.b, tint.a);
    rlNormal3f(0.0f, 0.0f, 1.0f);                          // Normal vector pointing towards viewer

    // Top-left corner for texture and quad
    if (flipX) rlTexCoord2f((source.x + source.width) / width, source.y / height);
    else rlTexCoord2f(source.x / width, source.y / height);
    rlVertex2f(topLeft.x, topLeft.y);

    // Bottom-left corner for texture and quad
    if (flipX) rlTexCoord2f((source.x + source.width) / width, (source.y + source.height) / height);
    else rlTexCoord2f(source.x / width, (source.y + source.height) / height);
    rlVertex2f(bottomLeft.x, bottomLeft.y);

    // Bottom-right corner for texture and quad
    if (flipX) rlTexCoord2f(source.x / width, (source.y + source.height) / height);
    else rlTexCoord2f((source.x + source.width) / width, (source.y + source.height) / height);
    rlVertex2f(bottomRight.x, bottomRight.y);

    // Top-right corner for texture and quad
    if (flipX) rlTexCoord2f(source.x / width, source.y / height);
    else rlTexCoord2f((source.x + source.width) / width, source.y / height);
    rlVertex2f(topRight.x, topRight.y);

    rlEnd();
    rlSetTexture(0);
}

// Read a line from memory
// REQUIRES: memcpy()
// NOTE: Returns the number of bytes read
static int GetLine(const char* origin, char* buffer, int maxLength)
{
	int count = 0;
	for (; count < maxLength; count++) if (origin[count] == '\n') break;
	memcpy(buffer, origin, count);
	return count;
}

// Load a BMFont file (AngelCode font file)
// REQUIRES: strstr(), sscanf(), strrchr(), memcpy()
Font LoadBMPFontFromTexture(const char* fileName, Texture2D* fontTexture, Vec2 offset)
{
	#define MAX_BUFFER_SIZE     256

	SASSERT(fileName);
	SASSERT(fontTexture);
	
	if (!FileExists(fileName))
	{
		SERR("BMFont file does not exist, %s", fileName);
	}

	if (!IsTextureReady(*fontTexture))
	{
		SWARN("Font texture atlas is not loaded!");
	}

	Font font = { 0 };

	char buffer[MAX_BUFFER_SIZE] = { 0 };
	char* searchPoint = NULL;

	int fontSize = 0;
	int glyphCount = 0;

	int imWidth = 0;
	int imHeight = 0;
	char imFileName[129] = { 0 };

	int base = 0;   // Useless data

	char* fileText = LoadFileText(fileName);

	if (fileText == NULL) return font;

	char* fileTextPtr = fileText;

	// NOTE: We skip first line, it contains no useful information
	int lineBytes = GetLine(fileTextPtr, buffer, MAX_BUFFER_SIZE);
	fileTextPtr += (lineBytes + 1);

	// Read line data
	lineBytes = GetLine(fileTextPtr, buffer, MAX_BUFFER_SIZE);
	searchPoint = strstr(buffer, "lineHeight");
	sscanf(searchPoint, "lineHeight=%i base=%i scaleW=%i scaleH=%i", &fontSize, &base, &imWidth, &imHeight);
	fileTextPtr += (lineBytes + 1);

	TRACELOGD("FONT: [%s] Loaded font info:", fileName);
	TRACELOGD("    > Base size: %i", fontSize);
	TRACELOGD("    > Texture scale: %ix%i", imWidth, imHeight);

	lineBytes = GetLine(fileTextPtr, buffer, MAX_BUFFER_SIZE);
	searchPoint = strstr(buffer, "file");
	sscanf(searchPoint, "file=\"%128[^\"]\"", imFileName);
	fileTextPtr += (lineBytes + 1);

	TRACELOGD("    > Texture filename: %s", imFileName);

	lineBytes = GetLine(fileTextPtr, buffer, MAX_BUFFER_SIZE);
	searchPoint = strstr(buffer, "count");
	sscanf(searchPoint, "count=%i", &glyphCount);
	fileTextPtr += (lineBytes + 1);

	TRACELOGD("    > Chars count: %i", glyphCount);

	// Compose correct path using route of .fnt file (fileName) and imFileName
	char* imPath = NULL;
	char* lastSlash = NULL;

	lastSlash = (char*)strrchr(fileName, '/');
	if (lastSlash == NULL) lastSlash = (char*)strrchr(fileName, '\\');

	if (lastSlash != NULL)
	{
		// NOTE: We need some extra space to avoid memory corruption on next allocations!
		imPath = (char*)RL_CALLOC(TextLength(fileName) - TextLength(lastSlash) + TextLength(imFileName) + 4, 1);
		memcpy(imPath, fileName, TextLength(fileName) - TextLength(lastSlash) + 1);
		memcpy(imPath + TextLength(fileName) - TextLength(lastSlash) + 1, imFileName, TextLength(imFileName));
	}
	else imPath = imFileName;

	TRACELOGD("    > Image loading path: %s", imPath);

	font.texture = *fontTexture;

	if (lastSlash != NULL) RL_FREE(imPath);

	Image imFont = LoadImageFromTexture(font.texture);

	// Fill font characters info data
	font.baseSize = fontSize;
	font.glyphCount = glyphCount;
	font.glyphPadding = 0;
	font.glyphs = (GlyphInfo*)RL_MALLOC(glyphCount * sizeof(GlyphInfo));
	font.recs = (Rectangle*)RL_MALLOC(glyphCount * sizeof(Rectangle));

	int charId, charX, charY, charWidth, charHeight, charOffsetX, charOffsetY, charAdvanceX;

	for (int i = 0; i < glyphCount; i++)
	{
		lineBytes = GetLine(fileTextPtr, buffer, MAX_BUFFER_SIZE);
		sscanf(buffer, "char id=%i x=%i y=%i width=%i height=%i xoffset=%i yoffset=%i xadvance=%i",
			&charId, &charX, &charY, &charWidth, &charHeight, &charOffsetX, &charOffsetY, &charAdvanceX);
		fileTextPtr += (lineBytes + 1);

		// Get character rectangle in the font atlas texture
		font.recs[i] = Rectangle{ (float)charX + offset.x, (float)charY + offset.y, (float)charWidth, (float)charHeight };

		// Save data properly in sprite font
		font.glyphs[i].value = charId;
		font.glyphs[i].offsetX = charOffsetX;
		font.glyphs[i].offsetY = charOffsetY;
		font.glyphs[i].advanceX = charAdvanceX;

		// Fill character image data from imFont data
		font.glyphs[i].image = ImageFromImage(imFont, font.recs[i]);
	}

	UnloadImage(imFont);
	UnloadFileText(fileText);

	if (font.texture.id == 0)
	{
		UnloadFont(font);
		font = GetFontDefault();
		TRACELOG(LOG_WARNING, "FONT: [%s] Failed to load texture, reverted to default font", fileName);
	}
	else TRACELOG(LOG_INFO, "FONT: [%s] Font loaded successfully (%i glyphs)", fileName, font.glyphCount);

	return font;
}
