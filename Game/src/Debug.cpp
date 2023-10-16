#include "Debug.h"

#include "Core.h"
#include "GameState.h"
#include "GUI.h"
#include "Components.h"

struct Debugger
{
	bool IsDebugMode;
	bool ShouldBreakOnWarning;
	bool IsPopupOpen;
	bool ShouldIgnorePopups;

	const char* ErrorPopupWindowMessage;
	const char* ErrorPopupWindowFile;
	const char* ErrorPopupWindowFunction;
	int ErrorPopupWindowLineNum;
	bool ErrorPopupWindowIsWarning;
};

internal_var struct Debugger Debugger;

void DrawDebugWindow(GameClient* client, GameState* gameState, GUIState* guiState)
{
	SAssert(client);
	SAssert(gameState);
	SAssert(guiState);

	if (client->IsDebugWindowOpen)
	{
		constexpr const char* WINDOW_NAME = "DEBUGGER";
		constexpr const float TEXT_ROW_HEIGHT = 24.0f;

		struct nk_context* ctx = &guiState->Ctx;

		float width = guiState->Scale.x;
		float height = guiState->Scale.y;

		ctx->style.window.fixed_background.data.color = {};

		struct nk_rect bounds = { 2.0f, 2.0f, width - 4.0f, height - 4.0f };
		if (nk_begin(ctx, WINDOW_NAME, bounds, NK_WINDOW_NO_SCROLLBAR))
		{
			nk_layout_space_begin(ctx, NK_STATIC, 0, INT_MAX);

			struct nk_color panelBgColor = { 100, 100, 100, 192 };
			ctx->style.window.fixed_background.data.color = panelBgColor;

			//
			// Selected Entity
			//
			nk_layout_space_push(ctx, nk_rect(1, 1, 512, 256));
			if (nk_group_begin(ctx, "EntityInfo", NK_WINDOW_NO_SCROLLBAR))
			{
				if (ecs_is_valid(gameState->World, client->SelectedEntity))
				{
					char* entityStr = ecs_entity_str(gameState->World, client->SelectedEntity);
					SAssert(entityStr);

					nk_layout_row_static(ctx, TEXT_ROW_HEIGHT * 3, 512, 1);
					nk_label_wrap(ctx, entityStr);

					nk_layout_row_static(ctx, TEXT_ROW_HEIGHT, 512, 1);
					if (ecs_has(gameState->World, client->SelectedEntity, CTransform))
					{
						const CTransform* transform = ecs_get(gameState->World, client->SelectedEntity, CTransform);
						nk_labelf(ctx, NK_TEXT_ALIGN_LEFT, "Pos = %s", FMT_VEC2(transform->Pos));
						nk_labelf(ctx, NK_TEXT_ALIGN_LEFT, "Tile = %s", FMT_VEC2I(transform->TilePos));
					}
				}
				nk_group_end(ctx);
			}

			//
			// Resource Info
			//
			nk_layout_space_push(ctx, nk_rect(width - 512 - 2, 1, 512, 256));
			if (nk_group_begin(ctx, "ResourceInfo", NK_WINDOW_NO_SCROLLBAR))
			{
				nk_layout_row_static(ctx, TEXT_ROW_HEIGHT, 512, 1);

				nk_labelf(ctx, NK_TEXT_ALIGN_LEFT, "FPS: %d", GetFPS());
				nk_labelf(ctx, NK_TEXT_ALIGN_LEFT, "UpdateTime: %.3fms", Client.UpdateTime * 1000.0);
				nk_labelf(ctx, NK_TEXT_ALIGN_LEFT, "DrawTime: %.3fms", GetDrawTime() * 1000.0);
				nk_labelf(ctx, NK_TEXT_ALIGN_LEFT, "GeneralPurpose: %dkb / %dkb",
						  (int)((gameState->GeneralPurposeMemory.Size - GeneralPurposeGetFreeMemory(&gameState->GeneralPurposeMemory)) / 1024),
						  (int)(gameState->GeneralPurposeMemory.Size / 1024));
				nk_labelf(ctx, NK_TEXT_ALIGN_LEFT, "GameArena: %dkb / %dkb",
						  (int)((gameState->GameArena.Size - ArenaSizeRemaining(&gameState->GameArena, 16)) / 1024),
						  (int)(gameState->GameArena.Size / 1024));
				nk_labelf(ctx, NK_TEXT_ALIGN_LEFT, "TempArena: %dkb / %dkb",
						  (int)((TransientState.TransientArena.Size - ArenaSizeRemaining(&TransientState.TransientArena, 16)) / 1024),
						  (int)(TransientState.TransientArena.Size / 1024));

				nk_group_end(ctx);
			}
			nk_layout_space_end(ctx);
		}
		nk_end(&guiState->Ctx);
	}
}

internal void
ResetDebugger()
{
	Debugger.ErrorPopupWindowMessage = nullptr;
	Debugger.ErrorPopupWindowFile = nullptr;
	Debugger.ErrorPopupWindowFunction = nullptr;
	Debugger.ErrorPopupWindowLineNum = 0;
	Debugger.IsPopupOpen = false;
	GetGameState()->IsGamePaused = false;
}

void TriggerErrorPopupWindow(bool isWarning, const char* errorMsg, const char* file, const char* function, int line)
{
	TraceLog((isWarning) ? LOG_WARNING : LOG_ERROR, errorMsg, NULL);

#if SWarnAlwaysAssert
	DebugBreak(void);
#endif // SWarnAlwaysAssert

	Debugger.ErrorPopupWindowIsWarning = isWarning;
	Debugger.ErrorPopupWindowMessage = errorMsg;
	Debugger.ErrorPopupWindowFile = file;
	Debugger.ErrorPopupWindowFunction = function;
	Debugger.ErrorPopupWindowLineNum = line;
	Debugger.IsPopupOpen = true;
	GetGameState()->IsGamePaused = true;

	if (Debugger.ShouldBreakOnWarning)
	{
		DebugBreak(void);

		ResetDebugger();
		Debugger.ShouldBreakOnWarning = false;
	}
}

void DrawErrorPopupWindow(GameState* gameState)
{
	SAssert(gameState);

	if (Debugger.IsPopupOpen && !Debugger.ShouldIgnorePopups)
	{
		nk_context* ctx = &gameState->GUIState.Ctx;
		float width = (float)GetScreenWidth();
		float height = (float)GetScreenHeight();

		struct nk_color panelBgColor = { 200, 200, 200, 255 };
		ctx->style.window.fixed_background.data.color = panelBgColor;
		struct nk_rect bounds;
		bounds.x = width / 2 - 250;
		bounds.y = height / 2 - 150;
		bounds.w = 500;
		bounds.h = 300;
		if (nk_begin(ctx, "ErrorWindow", bounds, NK_WINDOW_MOVABLE | NK_WINDOW_NO_SCROLLBAR))
		{
			nk_layout_row_static(ctx, bounds.h / 10.0f, (int)bounds.w, 1);

			nk_color textColor = { 0, 0, 0, 255 };

			nk_label_colored(ctx, (Debugger.ErrorPopupWindowIsWarning) ? "Warning occurred!" : "Error occured!", NK_TEXT_ALIGN_CENTERED, textColor);
			nk_labelf_colored_wrap(ctx, textColor, "Error: %s", (Debugger.ErrorPopupWindowMessage) ? Debugger.ErrorPopupWindowMessage : "Unknown error");
			nk_labelf_colored_wrap(ctx, textColor, "File: %s", (Debugger.ErrorPopupWindowFile) ? Debugger.ErrorPopupWindowFile : "Unknown file");
			nk_labelf_colored_wrap(ctx, textColor, "Function: %s", (Debugger.ErrorPopupWindowFunction) ? Debugger.ErrorPopupWindowFunction : "Unknown function");
			nk_labelf_colored_wrap(ctx, textColor, "Line: %d", Debugger.ErrorPopupWindowLineNum);

			nk_style_button style = ctx->style.button;
			style.text_alignment = NK_TEXT_CENTERED;

			nk_layout_row_dynamic(ctx, (bounds.h / 10.0f) * 4, 4);
			if (nk_button_label(ctx, "Continue"))
			{
				ResetDebugger();
			}
			if (nk_button_label(ctx, "Ignore popups"))
			{
				Debugger.ShouldIgnorePopups = true;
				ResetDebugger();
			}
			if (nk_button_label_styled(ctx, &style, "Break next warning"))
			{
				Debugger.ShouldBreakOnWarning = true;

				ResetDebugger();
			}
			if (nk_button_label(ctx, "Break now"))
			{
				DebugBreak(void);
				ResetDebugger();
			}
		}
		nk_end(ctx);
	}
}

Timer::Timer()
{
	End = 0;
	Name = nullptr;
	StartSecs = GetTime();
	Start = zpl_rdtsc();
}

Timer::Timer(const char* name)
{
	Name = name;
	End = 0;
	StartSecs = GetTime();
	Start = zpl_rdtsc();
}

Timer::~Timer()
{
	End = zpl_rdtsc() - Start;
	double EndSeconds = GetTime() - StartSecs;
	if (Name)
		SInfoLog("[ Timer ] Timer %s took %llu cycles, %f ns", Name, End, EndSeconds * 1000 * 1000);
	else
		SInfoLog("[ Timer ] Timer took %llu cycles, %f ns", End, EndSeconds * 1000 * 1000);
}
