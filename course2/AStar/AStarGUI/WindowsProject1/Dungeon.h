#pragma once
#include "IMap.h"

#define DUNGEON_WIDTH 100
#define DUNGEON_LENGTH 100

class Dungeon : public IMap{

public:
	//가로,세로를 받아서 초기화
	Dungeon(int y, int x) {

		for (int i = 0; i < y; i++)
		{
			for (int j = 0; j < x; j++)
			{
				map[i][j] = 0;
			}
		}
	}

	int CheckTile(int y, int x);

	void ChangeTile(int y, int x, int v);

private:
	int map[DUNGEON_LENGTH][DUNGEON_WIDTH] = {0,};

	bool IsObstacle(int y, int x);
};