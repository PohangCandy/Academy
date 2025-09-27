#pragma once
#include "IMap.h"

#define DUNGEON_WIDTH 1000
#define DUNGEON_LENGTH 1000

class Dungeon : public IMap{

public:
	//가로,세로를 받아서 초기화
	Dungeon(int y, int x) {

		_width = x;
		_height = y;

		InitMap();
	}

	ETileType CheckTile(int y, int x);

	void ChangeTile(int y, int x, ETileType v);

	int getwidth() { return  _width; }
	int getheight() { return  _height; }

	Grid* getStart() { return getGrid(_start.y, _start.x); }
	Grid* getGoal() { return getGrid(_goal.y, _goal.x); }

	Grid _start = { 0, 0 };
	Grid _goal = { 10, 0 };

	void mapUpdate()
	{
		map[_start.y][_start.x].type = start;
		map[_start.y][_start.x].y = _start.y;
		map[_start.y][_start.x].x = _start.x;
		map[_start.y][_start.x].g = 0;
		map[_start.y][_start.x].f = map[_start.y][_start.x].h;
		map[_goal.y][_goal.x].y = _goal.y;
		map[_goal.y][_goal.x].x = _goal.x;
		map[_goal.y][_goal.x].type = end;
	}

	//장애물, 출발지, 도착지를 제외한 모든 노드 초기화
	void InitMap()
	{
		mapUpdate();

		for (int y = 0; y < _height; y++)
		{
			for (int x = 0; x < _width; x++)
			{
				if (map[y][x].type == nodelist || map[y][x].type == shortest || map[y][x].type == visited) map[y][x].type = none;
				map[y][x].y = y;
				map[y][x].x = x;
				map[y][x].h = 0;
				map[y][x].g = 0;
				map[y][x].f = 0;
				map[y][x].gparent = nullptr;
			}
		}

	}

	float getGridFdata(int y, int x);
	Grid* getGrid(int y, int x);
	void setMapData(int y, int x, float g, float h, float F, Grid* parent);
	//Grid GetGridWithPos(int y, int x);

private:

	int _width = 0;

	int _height = 0;

	Grid map[DUNGEON_LENGTH][DUNGEON_WIDTH];

	bool IsObstacle(int y, int x);
};