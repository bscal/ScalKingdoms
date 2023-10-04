#define ZPL_IMPL
#include <zpl/zpl.h>

#define FNL_IMPL
#include <FastNoiseLite/FastNoiseLite.h>

// Defined in GameState.cpp
extern int GameInitialize();

int
main(int argCount, char** args)
{
	int result = GameInitialize();
	return result;
}