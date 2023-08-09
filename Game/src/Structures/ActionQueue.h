#pragma once

#include "Core.h"

struct ActionType;
struct Action;
struct CEntityAction;

typedef void(*PreAction)(ecs_entity_t, ActionType*, CEntityAction*);
typedef void(*StartAction)(ecs_entity_t, ActionType*, CEntityAction*);
typedef void(*UpdateAction)(ecs_entity_t, ActionType*, CEntityAction*);
typedef void(*EndAction)(ecs_entity_t, ActionType*, CEntityAction*);

struct ActionType
{
	PreAction OnPreAction;
	StartAction OnStartAction;
	UpdateAction OnUpdateAction;
	EndAction OnEndAction;

	u16 ProgressNeeded;
	u16 AttributeId;
};

struct Action
{
	u16 ActionTypeId;
	u16 Progress;
	union
	{
		struct Move
		{
			Vec2i Pos;
		};
		struct MeleeAtk
		{
			ecs_entity_t Entity;
		};
	};
};

struct CEntityAction
{
	Action CurrectAction;
	bool IsTask;
	bool CompleteTillFinished;
};

enum ActionPriority : i8
{
	ACTION_PRIORITY_0, // Done first
	ACTION_PRIORITY_1,
	ACTION_PRIORITY_2,
	ACTION_PRIORITY_3,
	ACTION_PRIORITY_4, // Done last

	ACTION_PRIORITY_MAX,

	ACTION_PRIORITY_WILL_NOT_DO = -1,
};

struct CEntityPriorityMap
{
	ActionPriority Priorities[1];
};

struct ActionMgr
{
	zpl_array(ActionType) ActionTypes;
};

void ActionMgrInitialize(ActionMgr* actionMgr);

namespace Actions
{
	inline u16 IDLE;
}
