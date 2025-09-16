#include "Dungeon.h"

ETileType Dungeon::CheckTile(int y, int x)
{
	return map[y][x];
}

void Dungeon::ChangeTile(int y, int x, ETileType v)
{
	map[y][x] = v;
}

bool Dungeon::IsObstacle(int y, int x)
{
	if (map[y][x] == obs)
	{
		return true;
	}
	return false;
}
