#include "Actions.h"

#include "Memory.h"
#include "GameState.h"
#include "Components.h"


void ActionMgrInitialize(ActionMgr* actionMgr)
{
	SAssert(actionMgr);

	ArrayListReserve(SAllocatorGeneral(), actionMgr->ActionTypes, 256);

	Actions::IDLE = (u16)ArrayListCount(actionMgr->ActionTypes);
	
	ActionType idle = {};
	idle.OnInitAction = [](Action* action, ecs_entity_t entity, u16 actionId)
	{
		SZero(action, sizeof(Action));
	};
	ArrayListPush(SAllocatorGeneral(), actionMgr->ActionTypes, idle);
}

internal u16
SearchForNextAction(ecs_entity_t entity, ActionMgr* actionMgr, CEntityPriorityMap* entityPiorities)
{
	return 0;
}

void SystemUpdateActions(ecs_iter_t* it)
{
	//CTransform* transforms = ecs_field(it, CTransform, 1);
	//CMove* moves = ecs_field(it, CMove, 2);
	CEntityPriorityMap* priorities = ecs_field(it, CEntityPriorityMap, 3);
	CEntityAction* actions = ecs_field(it, CEntityAction, 4);

	ActionMgr* actionMgr = &GetGameState()->ActionMgr;

	for (int i = 0; i < it->count; ++i)
	{
		CEntityAction* action = &actions[i];

		u16 actionid = action->CurrectAction.ActionTypeId;
		if (actionid > 0)
		{
			ActionType* actionType = &actionMgr->ActionTypes[actionid];

			// TODO: get bonuses for work
			u16 workProgress = 1;
			action->CurrectAction.Progress += workProgress;
			if (action->CurrectAction.Progress >= actionType->ProgressNeeded) // Action is complete!
			{
				u16 nextAction = 0;
				if (actionType->OnEndAction)
					nextAction = actionType->OnEndAction(it->entities[i], actionType, action);

				if (nextAction == 0) // No next action, search
				{
					nextAction = SearchForNextAction(it->entities[i], actionMgr, &priorities[i]);
				}

				// Applies next action
				ActionType* nextActionType = &actionMgr->ActionTypes[nextAction];
				SAssert(nextActionType->OnInitAction);
				nextActionType->OnInitAction(&action->CurrectAction, it->entities[i], nextAction);
			}
			else  // Update Action
			{
				if (actionType->OnUpdateAction)
					actionType->OnUpdateAction(it->entities[i], actionType, action);
			}
		}
		else
		{
			u16 nextAction = SearchForNextAction(it->entities[i], actionMgr, &priorities[i]);
			if (nextAction > 0)
			{
				// Applies next action if found 1
				ActionType* nextActionType = &actionMgr->ActionTypes[nextAction];
				SAssert(nextActionType->OnInitAction);
				nextActionType->OnInitAction(&action->CurrectAction, it->entities[i], nextAction);
			}
		}
	}
}
