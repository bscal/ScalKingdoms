#define RAYLIB_NUKLEAR_IMPLEMENTATION
#include "GUI.h"

#include <NuklearRaylib/raylib-nuklear.h>

#include "GameState.h"

internal_var NuklearUserData UserData;

bool 
InitializeGUI(GameState* gameState, Font* guiFont)
{
	Font* font = guiFont;

	gameState->GUIState.Font.userdata.ptr	= guiFont;
	gameState->GUIState.Font.height			= (float)font->baseSize;
	gameState->GUIState.Font.width			= CalculateTextWidth;

	gameState->GUIState.Ctx.clip.copy		= nk_raylib_clipboard_copy;
	gameState->GUIState.Ctx.clip.paste		= nk_raylib_clipboard_paste;
	gameState->GUIState.Ctx.clip.userdata	= nk_handle_ptr(0);

	size_t guiMemorySize = Megabytes(2);
	void* guiMemory = SAlloc(SAllocatorArena(&State.GameArena), guiMemorySize);

	if (!nk_init_fixed(&gameState->GUIState.Ctx, guiMemory, guiMemorySize, &gameState->GUIState.Font))
	{
		SError("[ UI ] Nuklear failed to initialized");
		return false;
	}

	// Set the internal user data.
	UserData.scaling = 1.0f;

	nk_handle userDataHandle = nk_handle_ptr(&UserData);
	nk_set_user_data(&gameState->GUIState.Ctx, userDataHandle);

	SInfoLog("[ UI ] Initialized Nuklear");
	return true;
}

void 
UpdateGUI(GameState* gameState)
{
	nk_context* ctx = &gameState->GUIState.Ctx;

	UpdateNuklear(ctx);
}

void 
DrawGUI(GameState* gameState)
{
	nk_context* ctx = &gameState->GUIState.Ctx;

	DrawNuklear(ctx);
}

float
CalculateTextWidth(nk_handle handle, float height, const char* text, int len)
{
	if (len == 0) return
		0.0f;

	SAssert(handle.ptr);
	SAssert(height > 0.0f);
	SAssert(text);

	Font* font = (Font*)handle.ptr;

	float baseSize = (float)font->baseSize;
	if (height < baseSize)
		height = baseSize;

	float spacing = height / baseSize;

	const char* subtext = TextSubtext(text, 0, len);

	Vector2 textSize = MeasureTextEx(*font, subtext, height, spacing);
	return textSize.x;
}
