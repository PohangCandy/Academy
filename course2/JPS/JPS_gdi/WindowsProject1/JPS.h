#pragma once
#include <iostream>
#include <list>
#include <set>
#include "Dungeon.h"

struct CompareGrid
{
	bool operator()(const Grid* a, const Grid* b) const
	{
		//일단 set을 망치지 않기 위해 이렇게 세팅해두고
		//나중에 성능좋은 자료구조로 다시 바꿔주자.
		if (a->f != b->f) return a->f < b->f; // F 기준
		if (a->g != b->g) return a->g < b->g; // tie-break
		if (a->h != b->h) return a->h < b->h; // tie-break
		if (a->x != b->x) return a->x < b->x;
		return a->y < b->y;
	}
};

class JPS {

public:
	Dungeon* _map;

	Grid* _startGrid;
	Grid* _goalGrid;

	//방문해야 할 리스트
	std::multiset <Grid*, CompareGrid>_openlist;

	JPS(Dungeon* mapinstance) : _map(mapinstance), _startGrid(nullptr), _goalGrid(nullptr)
	{
		g_rgb = 0;
		_goalGrid = _map->getGoal();
		_startGrid = _map->getStart();
	}

	~JPS()
	{
		makeInitList();
	}

	bool bfirst = true;
	int g_rgb;
	bool findPathwithRender();

	bool findPath();

	void updateNode();

	bool checkGridIsInMap(Grid g);

	void makeInitList();

private:
	bool CheckDiagonal(int x, int y, int dx, int dy);

	bool ExploreDirection(Grid* g, int dx, int dy);

	void setDirectionToTravel(Grid* g, EDirection d);

	void findNodeWithDirection(Grid* g);

	float findHbyGrid(int y, int x);

	void insertListEightDirection();
};