#pragma once

struct GameClient;
struct GameState;
struct GUIState;

#define InvalidCodePath SError("Invalid code path, %s, %s, %d", __FILE__, __FUNCTION__, __LINE__)
#define Success(var) (var)
#define Failure(var) (!(var))

void DrawDebugWindow(GameClient* client, GameState* gameState, GUIState* guiState);

void TriggerErrorPopupWindow(bool isWarning, const char* errorMsg, const char* file, const char* function, int line);
void DrawErrorPopupWindow(GameState* gameState);
