#include "Debug.h"

#include "Core.h"
#include "GameState.h"
#include "GUI.h"
#include "Components.h"

struct Debugger
{
	bool IsDebugMode;
};

global struct Debugger Debugger;

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
					(int)((gameState->GameMemory.Size - MemArenaGetFreeMemory(gameState->GameMemory)) / 1024),
					(int)(gameState->GameMemory.Size / 1024));
				nk_labelf(ctx, NK_TEXT_ALIGN_LEFT, "TempMem: %dkb / %dkb",
					(int)(gameState->FrameMemory.total_allocated / 1024),
					(int)(gameState->FrameMemory.total_size / 1024));

				nk_group_end(ctx);
			}
			nk_layout_space_end(ctx);
		}
		nk_end(&guiState->Ctx);
	}
}