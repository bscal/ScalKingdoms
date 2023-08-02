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

void DrawSprite(Texture2D* texture, Rectangle source, Rectangle dest, Vec2 origin, Color tint, bool flipX)
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