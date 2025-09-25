#include "Dungeon.h"

ETileType Dungeon::CheckTile(int y, int x)
{
	if (y < 0 || x < 0 || y >= _height || x >= _height) return out;

	return map[y][x];
}

void Dungeon::ChangeTile(int y, int x, ETileType v)
{
	map[y][x] = v;
}

bool Dungeon::IsObstacle(int y, int x)
{
	if (CheckTile(y, x) == obs || CheckTile(y, x) == out) return true;

	else 
		return false;
}
