#pragma once

#include "Core.h"
#include "Lib/String.h"

struct GameClient;
struct GUIState;

#define COMMAND_SUCCESS 0
#define COMMAND_FAILURE 1

#define CONSOLE_ONCOMMAND_PROC(name) int name(const String Command, const char** args, int argsLength)
typedef CONSOLE_ONCOMMAND_PROC(OnCommandProc);

struct Command
{
	OnCommandProc* OnCommand;
	String ArgumentString;
};

void ConsoleInit();

void ConsoleRegisterCommand(String cmdName, Command* cmd);

void ConsoleDraw(GameClient* client, GUIState* guiState);
