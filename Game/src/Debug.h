#pragma once

#include "Base.h"

struct GameClient;
struct GameState;
struct GUIState;

#define InvalidCodePath SError("Invalid code path, %s, %s, %d", __FILE__, __FUNCTION__, __LINE__)
#define Success(var) (var)
#define Failure(var) (!(var))

void DrawDebugWindow(GameClient* client, GameState* gameState, GUIState* guiState);

void TriggerErrorPopupWindow(bool isWarning, const char* errorMsg, const char* file, const char* function, int line);
void DrawErrorPopupWindow(GameState* gameState);

#define TimerStart(name) Timer(name)
#define TimerStartFunc() TimerStart(__FUNCTION__)

struct Timer
{
	u64 Start;
	u64 End;
	double StartSecs;
	const char* Name;

	Timer();

	Timer(const char* name);

	~Timer();
};
