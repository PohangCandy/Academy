#pragma once
#include "IMap.h"
#include <stdlib.h>

#define DUNGEON_WIDTH 150
#define DUNGEON_LENGTH 100

class Dungeon : public IMap{

public:
	//가로,세로를 받아서 초기화
	Dungeon(int y, int x) {

		_width = x;
		_height = y;

		InitMap();
	}

	~Dungeon() {};

	ETileType CheckTile(int y, int x)
	{
		if (y < 0 || x < 0 || y >= _height || x >= _width) return out;

		return map[y][x].type;
	}

	void ChangeTile(int y, int x, ETileType v)
	{
		if (v == none || v == obs)
		{
			map[y][x].h = 0;
			map[y][x].g = 0;
			map[y][x].f = 0;
		}
		map[y][x].type = v;
	}

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
		map[_start.y][_start.x].gparent = nullptr;
		map[_goal.y][_goal.x].y = _goal.y;
		map[_goal.y][_goal.x].x = _goal.x;
		map[_goal.y][_goal.x].type = goal;
		//map[_goal.y][_goal.x].gparent = nullptr;
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

	float getGridFdata(int y, int x)
	{
		return map[y][x].f;
	}

	Grid* getGrid(int y, int x)
	{
		return &map[y][x];
	}

	void setMapData(int y, int x, float g, float h, float F, Grid* parent, int g_rgb)
	{
		map[y][x].x = x;
		map[y][x].y = y;
		map[y][x].h = h;
		map[y][x].g = g;
		map[y][x].f = F;
		map[y][x].gparent = parent;
		map[y][x].rgb = g_rgb;
	}

	//Grid GetGridWithPos(int y, int x);
	int getRGB(int y, int x)
	{
		return map[y][x].rgb;
	}

	bool IsObstacle(int y, int x)
	{
		if (CheckTile(y, x) == obs || CheckTile(y, x) == out) return true;

		else
			return false;
	}

	void resetObs()
	{

		for (int y = 0; y < _height; y++)
		{
			for (int x = 0; x < _width; x++)
			{
				if (map[y][x].type == obs)
				{
					ChangeTile(y, x, none);
				}
			}
		}
	}

	void GenerateRandomMap(float obstacle_ratio)
	{

		resetObs(); // 기존 장애물을 모두 지움
		InitMap();  // Start/Goal 설정 및 경로 흔적 초기화

		int total_cells = _height * _width;
		// 장애물 비율이 0~1 사이인지 확인 후 개수 계산
		if (obstacle_ratio < 0.0f) obstacle_ratio = 0.0f;
		if (obstacle_ratio > 1.0f) obstacle_ratio = 1.0f;

		int obs_count = (int)(total_cells * obstacle_ratio);

		for (int i = 0; i < obs_count; ++i)
		{
			// 무작위 좌표 생성
			int y = rand() % _height;
			int x = rand() % _width;

			// Start 또는 Goal 지점은 장애물로 설정
			if (CheckTile(y, x) != start && CheckTile(y, x) != goal)
			{
				// 장애물로 설정하면 ChangeTile 내부에서 g, h, f가 0으로 초기화
				ChangeTile(y, x, obs);
			}
		}
		// 맵 상태 재확인 (Start/Goal이 최종적으로 덮어쓰이지 않도록)
		mapUpdate();
	}

private:

	int _width = 0;

	int _height = 0;

	Grid map[DUNGEON_LENGTH][DUNGEON_WIDTH];

	
};