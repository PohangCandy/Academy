#pragma once
#include "IMap.h"

#define MAP_WIDTH 100
#define MAP_LENGTH 100

class Dungeon : public IMap{

public:
	Dungeon() {
		for (int i = 0; i < 5; i++)
		{
			map[2][i] = 1;
		}
		map[1][4] = 1;
	}

private:
	int map[MAP_LENGTH][MAP_WIDTH] = {0,};

	bool IsObstacle(int y, int x);
};