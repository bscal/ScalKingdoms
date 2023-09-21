#include "Console.h"

#include "GameState.h"
#include "GUI.h"

#include "Structures/Queue.h"
#include "Structures/HashMapStr.h"

constexpr global_var int CONSOLE_ENTRY_LENGTH = 60;
constexpr global_var int CONSOLE_MAX_ENTRIES = 128;
constexpr global_var int CONSOLE_MAX_SUGGESTIONS = 4;
constexpr global_var int CONSOLE_NUM_SUGGESTIONS_ARGS = 6;

#define ENABLE_CONSOLE_LOGGING 1

#if ENABLE_CONSOLE_LOGGING
#define SLogConsole(msg, ...) SInfoLog("[ Commands ] " msg, __VA_ARGS__)
#else
#define SLogConsole(msg, ...)
#endif

internal void ConsoleFillSuggestions();

internal void ConsoleHandleCommand();

struct Console
{
	// Text entry
	int EntryLength; 
	char EntryString[CONSOLE_ENTRY_LENGTH];

	HashMapStr Commands;
	Queue<zpl_string> Entries;

	zpl_string* CurrentCommandArgString;
	zpl_string* Suggestions[CONSOLE_MAX_SUGGESTIONS];
	int NumOfSuggestions;
};

global_var struct Console Console;

void ConsoleInit()
{
	zpl_allocator allocator = ZplAllocatorMalloc;

	HashMapStrInitialize(&Console.Commands, sizeof(Command), 32, Allocator::Malloc);
	Console.Entries.Init(Allocator::Malloc, CONSOLE_MAX_ENTRIES);

	for (size_t i = 0; i < CONSOLE_MAX_ENTRIES; ++i)
	{
		Console.Entries.Memory[i] = zpl_string_make_reserve(allocator, CONSOLE_ENTRY_LENGTH);
	}
}

void ConsoleRegisterCommand(zpl_string cmdName, Command* cmd)
{
	HashMapStrSet(&Console.Commands, cmdName, cmd);

	SLogConsole("Registered new command: %s", cmdName);
}

void ConsoleDraw(GameClient* client, GUIState* guiState)
{
	SAssert(client);
	SAssert(guiState);

	local_static float HeightAnimValue;			// Height of frame when opening the console
	local_static float SuggestionPanelSize;		// There can be X amount of suggetions
	local_static bool WasConsoleAlreadyOpen;

	if (client->IsConsoleOpen)
	{
		if (!WasConsoleAlreadyOpen)
			HeightAnimValue = 0;

		WasConsoleAlreadyOpen = true;

		constexpr float CONSOLE_ANIM_SPEED = 900 * 6;
		constexpr float INPUT_HEIGHT = 36.0f;
		constexpr float INPUT_WIDTH = INPUT_HEIGHT + 12.0f;
		constexpr int SCROLL_BAR_OFFSET = 16;
		constexpr float FONT_SIZE = 16.0f;
		constexpr float TEXT_ENTRY_HEIGHT = 16.0f;
		constexpr int TEXT_ENTRY_HEIGHT_WITH_PADDING = (int)TEXT_ENTRY_HEIGHT + 4;
		constexpr float PANEL_SIDE_PANEL = 32.0f;
		constexpr float SUGGESTION_ROW_HEIGHT = 24.0f;

		float width = (float)GetScreenWidth() - (PANEL_SIDE_PANEL * 2.0f);
		float height = (float)GetScreenHeight() * .75f + SuggestionPanelSize;
		if (HeightAnimValue < height)
		{
			height = HeightAnimValue;
			HeightAnimValue += DeltaTime * CONSOLE_ANIM_SPEED;
		}

		nk_context* ctx = &guiState->Ctx;

		struct nk_color bgColor = { 117, 117, 117, 200 };
		ctx->style.window.fixed_background.data.color = bgColor;

		struct nk_rect bounds = { PANEL_SIDE_PANEL, 0.0f, width, height };
		if (nk_begin(ctx, "Console", bounds, NK_WINDOW_NO_SCROLLBAR))
		{
			if (!WasConsoleAlreadyOpen)
			{
				if (Console.Entries.Count >= height / TEXT_ENTRY_HEIGHT_WITH_PADDING)
				{
					nk_group_set_scroll(ctx, "Messages", 0, Console.Entries.Count * TEXT_ENTRY_HEIGHT_WITH_PADDING);
				}
			}

			nk_layout_row_static(ctx, height - INPUT_WIDTH - SuggestionPanelSize, (int)width - SCROLL_BAR_OFFSET, 1);

			if (nk_group_begin(ctx, "Messages", 0))
			{
				nk_layout_row_dynamic(ctx, TEXT_ENTRY_HEIGHT, 1);
				for (int i = 0; i < Console.Entries.Count; ++i)
				{
					nk_label(ctx, *Console.Entries.At(i), NK_TEXT_LEFT);
				}
				nk_group_end(ctx);
			}
			
			if (IsKeyPressed(KEY_TAB) && Console.NumOfSuggestions > 0)
			{
				memset(Console.EntryString, 0, CONSOLE_ENTRY_LENGTH);
				Console.EntryLength = (int)zpl_string_length(*Console.Suggestions[0]);
				memcpy(Console.EntryString, *Console.Suggestions[0], Console.EntryLength);
			}

			else if (IsKeyPressed(KEY_ENTER))
			{
				ConsoleHandleCommand();
				
				if (Console.Entries.IsFull())
					Console.Entries.PopFirst();

				zpl_string entryString = Console.Entries.Memory[Console.Entries.Last];
				SAssert(entryString);
				memcpy(entryString, Console.EntryString, Console.EntryLength);
				memset(Console.EntryString, 0, CONSOLE_ENTRY_LENGTH);
				Console.Entries.PushLast(&entryString);

				Console.EntryLength = 0;

				if (Console.Entries.Count >= height / TEXT_ENTRY_HEIGHT_WITH_PADDING)
				{
					nk_group_set_scroll(ctx, "Messages", 0, Console.Entries.Count * TEXT_ENTRY_HEIGHT_WITH_PADDING);
				}
			}

			// *** Command Input ***
			nk_layout_row_begin(ctx, NK_STATIC, INPUT_HEIGHT, 2);

			float textWidth = CalculateTextWidth(ctx->style.font->userdata, FONT_SIZE, Console.EntryString, Console.EntryLength) + FONT_SIZE;
			float barWidth = nk_window_get_width(ctx);
			float barWidthMin = barWidth / 2;
			if (textWidth < barWidthMin)
				textWidth = barWidthMin;
			float argWidth = barWidth - textWidth;

			nk_layout_row_push(ctx, textWidth);

			nk_edit_string(ctx,
				NK_EDIT_FIELD,
				Console.EntryString,
				&Console.EntryLength,
				CONSOLE_ENTRY_LENGTH,
				nk_filter_default);

			SuggestionPanelSize = 0;
			if (Console.EntryLength > 0)
			{
				ConsoleFillSuggestions();

				SuggestionPanelSize = (float)(Console.NumOfSuggestions) * SUGGESTION_ROW_HEIGHT;

				nk_layout_row_push(ctx, argWidth);
				if (Console.CurrentCommandArgString)
				{
					nk_label_colored(ctx, *Console.CurrentCommandArgString, NK_TEXT_LEFT, { 255, 0, 0, 255 });
				}
				nk_layout_row_end(ctx);
				
				nk_layout_row_dynamic(ctx, FONT_SIZE, 1);
				for (int i = 0; i < Console.NumOfSuggestions; ++i)
				{
					nk_label(ctx, *Console.Suggestions[i], NK_TEXT_LEFT);
				}
			}
			else
				nk_layout_row_end(ctx);
		}
		nk_end(&guiState->Ctx);

		nk_window_set_focus(ctx, "Console");
	}
	else if (WasConsoleAlreadyOpen)
		WasConsoleAlreadyOpen = false;
}

internal void 
ConsoleFillSuggestions()
{
	Console.NumOfSuggestions = 0;
	Console.CurrentCommandArgString = nullptr;

	const char* firstSpace = strchr((const char*)Console.EntryString, ' ');

	int idx = 0;
	if (!firstSpace)
		idx = Console.EntryLength;
	else
		idx = (int)(firstSpace - (char*)Console.EntryString);

	zpl_string tempString = zpl_string_make_length(ZplAllocatorFrame, Console.EntryString, idx);

	for (u32 i = 0; i < Console.Commands.Capacity; ++i)
	{
		if (Console.Commands.Buckets[i].IsUsed)
		{
			const char* foundStr = strstr(Console.Commands.Buckets[i].String, tempString);
			if (foundStr)
			{
				Console.Suggestions[Console.NumOfSuggestions] = &Console.Commands.Buckets[i].String;

				++Console.NumOfSuggestions;
				if (Console.NumOfSuggestions == CONSOLE_MAX_SUGGESTIONS)
					break;
			}
		}
	}

	Command* cmd = (Command*)HashMapStrGet(&Console.Commands, tempString);
	if (cmd)
	{
		Console.CurrentCommandArgString = &cmd->ArgumentString;
	}
}

internal void 
ConsoleHandleCommand()
{
	const char* firstSpace = strchr((const char*)Console.EntryString, ' ');

	int idx = 0;
	if (!firstSpace)
		idx = Console.EntryLength;
	else
		idx = (int)(firstSpace - (char*)Console.EntryString);

	zpl_string tempString = zpl_string_make_length(ZplAllocatorFrame, Console.EntryString, idx);
	Command* cmd = (Command*)HashMapStrGet(&Console.Commands, tempString);
	if (cmd)
	{
		int argCount = 0;
		const char** split = TextSplit((const char*)(Console.EntryString) + idx, ' ', &argCount);

		// argCount will always be 1
		if (argCount > 0)
			--argCount;

		int cmdResult = cmd->OnCommand(tempString, split, argCount);
		if (cmdResult != 0)
			SWarn("[ Commands ] Command (%s) ran with error: %d", tempString, cmdResult);
	}
}