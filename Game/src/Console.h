#pragma once

#include "Core.h"

struct GameClient;
struct GUIState;

struct Command
{
	int(*OnCommand)(zpl_string Command, const char** args, int argsLength);
	zpl_string ArgumentString;
};

void ConsoleInit();

void ConsoleRegisterCommand(zpl_string cmdName, Command* cmd);

void ConsoleDraw(GameClient* client, GUIState* guiState);
