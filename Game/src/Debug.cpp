#include "Debug.h"

#include "Core.h"
#include "GameState.h"
#include "GUI.h"
#include "Components.h"

struct Debugger
{
	bool IsDebugMode;
	bool ShouldBreakOnWarning;

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

		float width = (float)GetScreenWidth();
		float height = (float)GetScreenHeight();
		
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

				nk_labelf(ctx, NK_TEXT_ALIGN_LEFT, "UpdateTime: %.3fms", Client.UpdateTime * 1000.0);
				nk_labelf(ctx, NK_TEXT_ALIGN_LEFT, "DrawTime: %.3fms", GetDrawTime() * 1000.0);
				nk_labelf(ctx, NK_TEXT_ALIGN_LEFT, "ArenaMem: %dkb / %dkb",
					(int)((gameState->GameMemory.Size - FreelistGetFreeMemory(&gameState->GameMemory)) / 1024),
					(int)(gameState->GameMemory.Size / 1024));
				nk_labelf(ctx, NK_TEXT_ALIGN_LEFT, "TempMem: %dkb / %dkb",
					(int)((gameState->FrameMemory.Size - LinearArenaFreeSpace(&gameState->FrameMemory)) / 1024),
					(int)(gameState->FrameMemory.Size / 1024));

				nk_group_end(ctx);
			}
			nk_layout_space_end(ctx);
		}
		nk_end(&guiState->Ctx);
	}
}

void TriggerErrorPopupWindow(bool isWarning, const char* errorMsg, const char* file, const char* function, int line)
{
	Debugger.ErrorPopupWindowIsWarning = isWarning;
	Debugger.ErrorPopupWindowMessage = errorMsg;
	Debugger.ErrorPopupWindowFile = file;
	Debugger.ErrorPopupWindowFunction = function;
	Debugger.ErrorPopupWindowLineNum = line;
	Client.IsErrorWindowOpen = true;
	GetGameState()->IsGamePaused = true;
	if (Debugger.ShouldBreakOnWarning)
	{
		DebugBreak(void);

		Debugger.ErrorPopupWindowMessage = nullptr;
		Debugger.ErrorPopupWindowFile = nullptr;
		Debugger.ErrorPopupWindowFunction = nullptr;
		Debugger.ErrorPopupWindowLineNum = 0;
		Debugger.ShouldBreakOnWarning = false;
		GetGameState()->IsGamePaused = false;
		Client.IsErrorWindowOpen = false;
	}
}

void DrawErrorPopupWindow(GameState* gameState)
{
	SAssert(gameState);

	if (Client.IsErrorWindowOpen)
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
		if (nk_begin(ctx, "ErrorWindow", bounds, NK_WINDOW_NO_SCROLLBAR))
		{
			nk_layout_row_static(ctx, bounds.h / 10.0f, (int)bounds.w, 1);

			nk_label(ctx, (Debugger.ErrorPopupWindowIsWarning) ? "Warning occurred!" : "Error occured!", NK_TEXT_ALIGN_CENTERED);
			nk_labelf(ctx, NK_TEXT_ALIGN_CENTERED, "Error: %s", (Debugger.ErrorPopupWindowMessage) ? Debugger.ErrorPopupWindowMessage : "Unknown error");
			nk_labelf(ctx, NK_TEXT_ALIGN_CENTERED, "File: %s", (Debugger.ErrorPopupWindowFile) ? Debugger.ErrorPopupWindowFile : "Unknown file");
			nk_labelf(ctx, NK_TEXT_ALIGN_CENTERED, "Function: %s", (Debugger.ErrorPopupWindowFunction) ? Debugger.ErrorPopupWindowFunction : "Unknown function");
			nk_labelf(ctx, NK_TEXT_ALIGN_CENTERED, "Line: %d", Debugger.ErrorPopupWindowLineNum);

			nk_layout_row_dynamic(ctx, (bounds.h / 10.0f) * 3, 3);
			if (nk_button_label(ctx, "Continue"))
			{
				goto Cleanup;
			}
			if (nk_button_label(ctx, "Break next warning"))
			{
				Debugger.ShouldBreakOnWarning = true;

				goto Cleanup;
			}
			if (nk_button_label(ctx, "Break now"))
			{
				DebugBreak(void);

			Cleanup:
				Debugger.ErrorPopupWindowMessage = nullptr;
				Debugger.ErrorPopupWindowFile = nullptr;
				Debugger.ErrorPopupWindowFunction = nullptr;
				Debugger.ErrorPopupWindowLineNum = 0;
				Client.IsErrorWindowOpen = false;
				GetGameState()->IsGamePaused = false;
			}
		}
		nk_end(ctx);
	}
}
