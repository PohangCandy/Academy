#include "Dungeon.h"

bool Dungeon::IsObstacle(int y, int x)
{
	if (map[y][x])
	{
		return true;
	}
	return false;
}
