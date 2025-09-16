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
					map[y][x] = start;
				}
				else if (y == _goal.y && x == _goal.x)
				{
					map[y][x] = end;
				}
				else
				{
					map[y][x] = none;
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
		map[_start.y][_start.x] = start;
		map[_goal.y][_goal.x] = end;
	}

	//장애물, 출발지, 도착지를 제외한 모든 노드 초기화
	void InitMap()
	{
		for (int y = 0; y < _height; y++)
		{
			for (int x = 0; x < _width; x++)
			{
				if (map[y][x] == start || map[y][x] == end || map[y][x] == obs) continue;
				map[y][x] = none;
			}
		}
	}

private:

	int _width = 0;

	int _height = 0;

	ETileType map[DUNGEON_LENGTH][DUNGEON_WIDTH] = { none,};

	bool IsObstacle(int y, int x);
};