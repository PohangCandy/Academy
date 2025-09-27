#pragma once
#include <iostream>
#include <list>
#include <set>
//#include "IMap.h"
#include "Dungeon.h"
//#define ASTAR_WIDTH 100
//#define ASTAR_Length 100

//int map[length][width];



//struct Grid {
//	int x;
//	int y;
//	
//	bool operator == (Grid a)
//	{
//		return (this->x == a.x) && (this->y == a.y);
//	}
//};

//class Node {
//public:
//	Grid pos = {0,0};
//	Node* parent = nullptr;
//	float G = 0; //출발점으로부터의 이동 거리
//	float H = 0; //목적지까지의 거리(장애물을 신경쓰지 않은 직선 거리)
//	//즉 출발지와 가장 가깝고
//	//목적지와 가장 가까운
//	//F가 최솟값인 노드를 우선으로 탐색
//	float F = 0; //G + H
//	
//	Node(){}
//
//	Node(const Node& other) {
//		pos.x = other.pos.x;
//		pos.y = other.pos.y;
//		G = other.G;
//		H = other.H;
//		parent = other.parent;
//		//parent = new Node(*other.parent); // 깊은 복사가 필요하다면 new Node(*other.parent) 처리를 해야 함
//		F = other.F;
//	}
//};

struct CompareGrid
{
	bool operator()(const Grid* a, const Grid* b) const
	{
		//일단 set을 망치지 않기 위해 이렇게 세팅해두고
		//나중에 성능좋은 자료구조로 다시 바꿔주자.
		if (a->f != b->f) return a->f < b->f; // F 기준
		if (a->h != b->h) return a->h < b->h; // tie-break
		if (a->x != b->x) return a->x < b->x;
		return a->y < b->y;
	}
};

class JPS {

public:
	Dungeon* _map;

	//Grid _start;
	//Grid _destination;

	Grid* _startGrid;
	Grid* _goalGrid;

	//방문해야 할 리스트
//우선 순위 큐로 했더니 openlist에 이미 방문한 노드가 있을 경우 탐색을 할 수 없음.
//1. 안정성을 위해 먼저 set으로 
	std::multiset <Grid*, CompareGrid>_openlist;
	//std::list <Node*>_closelist;
	//std::list <Node*>_shortestRoutelist;

	//AStar()
	//{
	//	Grid start = { 0,0 };
	//	_start = start;

	//}

	JPS(Dungeon* mapinstance) : _map(mapinstance), _startGrid(nullptr), _goalGrid(nullptr)
	{

		//_start = start;
		//_destination = destination;
		_goalGrid = _map->getGoal();
		_startGrid = _map->getStart();
	}

	~JPS()
	{
		makeInitList();
		//startNode는 closeList에서 자동으로 삭제 되는 오류 주의
		//delete _startGrid;
		//delete _goalGrid;
	}

	bool bfirst = true;
	int g_rgb;
	bool findPathwithRender();

	bool findPath();

	void updateNode();

	//bool checkNodeIsInMap(Node* n);

	bool checkGridIsInMap(Grid g);

private:
	bool CheckDiagonal(int x, int y, int dx, int dy);

	bool ExploreDirection(Grid* g, int dx, int dy);

	void setDirectionToTravel(Grid* g, EDirection d);

	void findNodeWithDirection(Grid* g);

	float findHbyGrid(int y, int x);
	//float findGbyNode(Node* s, Node* d);
	//float findF(Node* n);

	void insertListEightDirection();

	void makeInitList();
};