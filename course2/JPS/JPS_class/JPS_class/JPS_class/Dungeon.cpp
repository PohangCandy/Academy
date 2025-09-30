#include "Dungeon.h"

//ETileType Dungeon::CheckTile(int y, int x)
//{
//	if (y < 0 || x < 0 || y >= _height || x >= _width) return out;
//
//	return map[y][x].type;
//}

//void Dungeon::ChangeTile(int y, int x, ETileType v)
//{
//	if (v == none || v == obs)
//	{
//		map[y][x].h = 0;
//		map[y][x].g = 0;
//		map[y][x].f = 0;
//	}
//	map[y][x].type = v;
//}

//float Dungeon::getGridFdata(int y, int x)
//{
//	return map[y][x].f;
//}

//Grid* Dungeon::getGrid(int y, int x)
//{
//	return &map[y][x];
//}

//void Dungeon::setMapData(int y, int x, float g, float h, float F, Grid* parent, int g_rgb)
//{
//	map[y][x].x = x;
//	map[y][x].y = y;
//	map[y][x].h = h;
//	map[y][x].g = g;
//	map[y][x].f = F;
//	map[y][x].gparent = parent;
//	map[y][x].rgb = g_rgb;
//}
//
//int Dungeon::getRGB(int y, int x)
//{
//	return map[y][x].rgb;
//}


//bool Dungeon::IsObstacle(int y, int x)
//{
//	if (CheckTile(y, x) == obs || CheckTile(y, x) == out) return true;
//
//	else 
//		return false;
//}
