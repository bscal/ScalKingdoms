#pragma once

#include "Core.h"
#include "Structures/ArrayList.h"

struct ActionType;
struct Action;
struct CEntityAction;

typedef void(*InitAction)(Action*, ecs_entity_t, u16);
typedef bool(*PreAction)(ecs_entity_t, ActionType*, CEntityAction*);
typedef bool(*StartAction)(ecs_entity_t, ActionType*, CEntityAction*);
typedef bool(*UpdateAction)(ecs_entity_t, ActionType*, CEntityAction*);
typedef u16(*EndAction)(ecs_entity_t, ActionType*, CEntityAction*);

struct ActionType
{
	InitAction OnInitAction;
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
		struct
		{
			Vec2i Pos;
		} Move;
		struct
		{
			ecs_entity_t Entity;
		} AtkMelee;
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
	ArrayList(ActionType) ActionTypes;
};

void ActionMgrInitialize(ActionMgr* actionMgr);

namespace Actions
{
	inline u16 IDLE;
}
