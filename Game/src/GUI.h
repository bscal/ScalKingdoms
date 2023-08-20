#pragma once

#include "Core.h"

#include <NuklearRaylib/raylib-nuklear.h>

struct GameState;

struct GUIState
{
	nk_context Ctx;
	nk_user_font Font;
};

bool InitializeGUI(GameState* gameState, Font* font);

void UpdateGUI(GameState* gameState);

void DrawGUI(GameState* gameState);
