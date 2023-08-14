#define ZPL_IMPL
#include <zpl/zpl.h>

#define FNL_IMPL
#include <FastNoiseLite/FastNoiseLite.h>

#include "GameEntry.h"

int
main(int argCount, char** args)
{
	int result = GameInitialize();
	return result;
}