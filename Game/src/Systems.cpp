#include "Systems.h"

#include "Components.h"

void DrawEntities(ecs_iter_t* it)
{
	CTransform transform = ecs_field(it, CTransform, 1);
	CRender render = ecs_field(it, CRender, 2);

	for (int i = 0; i < it->count; ++i)
	{

	}
}
