#pragma once

#include "Core.h"
#include "Lib/String.h"

struct GameClient;
struct GUIState;

#define COMMAND_SUCCESS 0

struct Command
{
	int(*OnCommand)(const String Command, const char** args, int argsLength);
	String ArgumentString;
};

void ConsoleInit();

void ConsoleRegisterCommand(String cmdName, Command* cmd);

void ConsoleDraw(GameClient* client, GUIState* guiState);
