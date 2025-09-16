#pragma once
#include <iostream>
#include <list>
#include <set>
#include "IMap.h"
//#define ASTAR_WIDTH 100
//#define ASTAR_Length 100

//int map[length][width];

enum EDirection
{
	LL, LU, UU, RU, RR, RD, DD, LD
};

//struct Grid {
//	int x;
//	int y;
//	
//	bool operator == (Grid a)
//	{
//		return (this->x == a.x) && (this->y == a.y);
//	}
//};

struct Node {
	Grid pos;
	Node* parent;
	float G; //출발점으로부터의 이동 거리
	float H; //목적지까지의 거리(장애물을 신경쓰지 않은 직선 거리)
	//즉 출발지와 가장 가깝고
	//목적지와 가장 가까운
	//F가 최솟값인 노드를 우선으로 탐색
	float F; //G + H

	EDirection  getNodedirection()
	{
		if (this->pos.x > parent->pos.x)
		{
			if (this->pos.y > parent->pos.y)
			{
				return RD;
			}
			else if (this->pos.y < parent->pos.y)
			{
				return RU;
			}
			else
			{
				return RR;
			}
		}
		else if(this->pos.x < parent->pos.x)
		{
			if (this->pos.y > parent->pos.y)
			{
				return LD;
			}
			else if (this->pos.y < parent->pos.y)
			{
				return LU;
			}
			else
			{
				return LL;
			}
		}
		else
		{
			if (this->pos.y > parent->pos.y)
			{
				return UU;
			}
			else
			{
				return DD;
			}
		}
	}
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

class JPS {

public:
	IMap* _map;

	//Grid _start;
	//Grid _destination;

	Node* _startNode;
	Node* _goalNode;

	//방문해야 할 리스트
//우선 순위 큐로 했더니 openlist에 이미 방문한 노드가 있을 경우 탐색을 할 수 없음.
//1. 안정성을 위해 먼저 set으로 
	std::multiset <Node*, CompareNode>_openlist;
	std::list <Node*>_closelist;
	std::list <Node*>_shortestRoutelist;

	//AStar()
	//{
	//	Grid start = { 0,0 };
	//	_start = start;

	//}

	JPS(IMap* mapinstance) : _map(mapinstance), _startNode(nullptr), _goalNode(nullptr)
	{

		//_start = start;
		//_destination = destination;
		_goalNode = new Node;
		_goalNode->pos = _map->getGoal();

		_startNode = new Node;
		_startNode->pos = _map->getStart();
		_startNode->parent = nullptr;
		_startNode->G = 0;
		_startNode->H = findHbyGrid(&_startNode->pos);
	}

	~JPS()
	{
		makeInitList();
		//startNode는 closeList에서 자동으로 삭제 되는 오류 주의
		delete _startNode;
		delete _goalNode;
	}

	bool findPath();

	void updateNode();

	bool checkNodeIsInMap(Node* n);

	bool checkGridIsInMap(Grid g);

private:

	void setDirectionToTravel(Node* n, EDirection d);

	void findNodeWithDirection(Node* n);

	float findHbyGrid(Grid* s);
	float findGbyNode(Node* s, Node* d);
	float findF(Node* n);

	void insertListEightDirection(Node* startNode);

	void makeInitList();
};