#include "ActionQueue.h"

#define Count ((u16)zpl_array_count(actionMgr->ActionTypes))
#define Append(var) (zpl_array_append(actionMgr->ActionTypes, var))

void ActionMgrInitialize(ActionMgr* actionMgr)
{
	SASSERT(actionMgr);

	zpl_array_init(actionMgr->ActionTypes, zpl_heap_allocator());

	Actions::IDLE = Count;
	
	ActionType idle = {};
	Append(idle);
}