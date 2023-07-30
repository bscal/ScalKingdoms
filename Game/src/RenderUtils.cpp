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