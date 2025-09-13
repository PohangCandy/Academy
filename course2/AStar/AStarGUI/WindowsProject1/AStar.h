#pragma once
#include <iostream>
#include <list>
#include <set>
#include "IMap.h"
#define ASTAR_WIDTH 100
#define ASTAR_Length 100

//int map[length][width];

struct Grid {
	int x;
	int y;

	bool operator == (Grid a)
	{
		return (this->x == a.x) && (this->y == a.y);
	}
};

struct Node {
	Grid pos;
	Node* parent;
	float G; //출발점으로부터의 이동 거리
	float H; //목적지까지의 거리(장애물을 신경쓰지 않은 직선 거리)
	//즉 출발지와 가장 가깝고
	//목적지와 가장 가까운
	//F가 최솟값인 노드를 우선으로 탐색
	float F; //G + H
};

struct CompareNode
{
	bool operator()(const Node* a, const Node* b) const
	{
		//일단 set을 망치지 않기 위해 이렇게 세팅해두고
		//나중에 성능좋은 자료구조로 다시 바꿔주자.
		if (a->F != b->F) return a->F < b->F; // F 기준
		if (a->H != b->H) return a->H < b->H; // tie-break
		if (a->pos.x != b->pos.x) return a->pos.x < b->pos.x;
		return a->pos.y < b->pos.y;
	}
};

class AStar {

public:
	IMap* _map;

	Grid _start;
	Grid _destination;

	Node* _startNode;
	Node* _goalNode;

	//방문해야 할 리스트
//우선 순위 큐로 했더니 openlist에 이미 방문한 노드가 있을 경우 탐색을 할 수 없음.
//1. 안정성을 위해 먼저 set으로 
	std::set <Node*, CompareNode>_openlist;
	std::list <Node*>_closelist;
	std::list <Node*>_shortestRoutelist;

	//AStar()
	//{
	//	Grid start = { 0,0 };
	//	_start = start;

	//}

	AStar(Grid start, Grid destination, IMap* mapinstance) : _map(mapinstance), _startNode(nullptr), _goalNode(nullptr)
	{

		_start = start;
		_destination = destination;

		_startNode = new Node;
		_startNode->pos = start;
		_startNode->parent = nullptr;
		_startNode->G = 0;
		_startNode->H = findHbyGrid(&_startNode->pos);

		_goalNode = new Node;
		_goalNode->pos = _destination;
	}

	~AStar()
	{
		makeEmptyList();
	}

	bool findPath();


	void updateNode();

private:

	float findHbyGrid(Grid* s);
	float findGbyNode(Node* s, Node* d);
	float findF(Node* n);

	void insertListEightDirection(Node* startNode);

	void makeEmptyList();
};