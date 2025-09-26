#include "Dungeon.h"

ETileType Dungeon::CheckTile(int y, int x)
{
	if (y < 0 || x < 0 || y >= _height || x >= _height) return out;

	return map[y][x].type;
}

void Dungeon::ChangeTile(int y, int x, ETileType v)
{
	map[y][x].type = v;
}

float Dungeon::getGridFdata(int y, int x)
{
	return map[y][x].f;
}

void Dungeon::setMapData(int y, int x, float g, float h, float F)
{
	map[y][x].x = x;
	map[y][x].y = y;
	map[y][x].h = h;
	map[y][x].g = g;
	map[y][x].f = F;
}

bool Dungeon::IsObstacle(int y, int x)
{
	if (CheckTile(y, x) == obs || CheckTile(y, x) == out) return true;

	else 
		return false;
}
