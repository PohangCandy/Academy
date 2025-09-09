#include "Dungeon.h"

int Dungeon::CheckTile(int y, int x)
{
	return map[y][x];
}

void Dungeon::ChangeTile(int y, int x, int v)
{
	map[y][x] = v;
}

bool Dungeon::IsObstacle(int y, int x)
{
	if (map[y][x])
	{
		return true;
	}
	return false;
}
