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
				map[i][j] = 0;
			}
		}
	}

	int CheckTile(int y, int x);

	void ChangeTile(int y, int x, int v);

	int getwidth() { return  _width; }
	int getheight() { return  _height; }

private:

	int _width = 0;

	int _height = 0;

	int map[DUNGEON_LENGTH][DUNGEON_WIDTH] = {0,};

	bool IsObstacle(int y, int x);
};