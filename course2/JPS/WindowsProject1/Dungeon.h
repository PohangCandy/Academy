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

		for (int i = 0; i < _height; i++)
		{
			for (int j = 0; j < _width; j++)
			{
				map[i][j] = none;
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

private:

	int _width = 0;

	int _height = 0;

	ETileType map[DUNGEON_LENGTH][DUNGEON_WIDTH] = { none,};

	bool IsObstacle(int y, int x);
};