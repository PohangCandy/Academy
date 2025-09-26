#pragma once
#include "IMap.h"

#define DUNGEON_WIDTH 100
#define DUNGEON_LENGTH 100

class Dungeon : public IMap{

public:
	//가로,세로를 받아서 초기화
	Dungeon(int y, int x) {

		_width = x;
		_height = y;

		for (int y = 0; y < _height; y++)
		{
			for (int x = 0; x < _width; x++)
			{
				if (y == _start.y && x == _start.x)
				{
					map[y][x].type = start;
				}
				else if (y == _goal.y && x == _goal.x)
				{
					map[y][x].type = end;
				}
				else
				{
					map[y][x].type = none;
				}
			}
		}
	}

	ETileType CheckTile(int y, int x);

	void ChangeTile(int y, int x, ETileType v);

	int getwidth() { return  _width; }
	int getheight() { return  _height; }

	Grid getStart() { return _start; }
	Grid getGoal() { return _goal; }

	Grid _start = { 0, 0 };
	Grid _goal = { 10, 0 };

	void mapUpdate()
	{
		map[_start.y][_start.x].type = start;
		map[_goal.y][_goal.x].type = end;
	}

	//장애물, 출발지, 도착지를 제외한 모든 노드 초기화
	void InitMap()
	{
		for (int y = 0; y < _height; y++)
		{
			for (int x = 0; x < _width; x++)
			{
				if (map[y][x].type == start || map[y][x].type == end || map[y][x].type == obs) continue;
				map[y][x].type = none;
				map[y][x].h = 0;
				map[y][x].g = 0;
				map[y][x].f = 0;
			}
		}
	}

	float getGridFdata(int y, int x);
	void setMapData(int y, int x, float g, float h, float F);

private:

	int _width = 0;

	int _height = 0;

	Grid map[DUNGEON_LENGTH][DUNGEON_WIDTH];

	bool IsObstacle(int y, int x);
};