#include "AStar.h"
#include <iostream>
using namespace std;

//지금 출발지와 도착지가 같을 경우, 출발지를 제외한 8방향으로 탐색을 시작하기 때문에 f값이 동일해
//8개중 랜덤한 노드가 8방향을 탐사하며 목적지를 찾게되므로 불필요한 탐사가 일어나는 것처럼 보임.
//하지만 정상
//이걸 바꾸려면 8방향 탐사를 임의로 하지 않도록 알고리즘을 수정해야 함.

//일단 제일 처음 좌표에서 도착지까지 H는 계산되야 함.
//뉴클리드 계산
float AStar::findGbyNode(Node* s, Node* d)
{
	//대각선으로 이동한 경우
	if (abs(s->pos.x - d->pos.x) + abs(s->pos.y - d->pos.y) == 2) return s->G + 1.4;
	//직선으로 이동한 경우
	else return s->G + 1;
}

float AStar::findF(Node* n)
{
	return n->H + n->G;
}

//도착지가 맴버로 저장되어있으므로 출발지만 갱신하면서 재귀해주면 될 것으로 보임.
//탐색한 경로를 리스트에 담으면서 f가 가장 적은 곳을 먼저 탐색하도록 만든다.
void AStar::insertListEightDirection(Node* sn)
{
	int dx[8] = { 0,1,1,1,0,-1,-1,-1 };
	int dy[8] = { 1,1,0,-1,-1,-1,0,1 };

	for (int i = 0; i < 8; i++)
	{
		int nx = sn->pos.x + dx[i];
		int ny = sn->pos.y + dy[i];
		if (nx < 0 || ny < 0 || nx >= ASTAR_WIDTH || ny >= ASTAR_Length) continue;

		Node* newNode = new Node;
		newNode->parent = sn;
		newNode->pos.x = nx;
		newNode->pos.y = ny;
		newNode->G = findGbyNode(sn, newNode);
		newNode->H = findHbyGrid(&newNode->pos);
		newNode->F = findF(newNode);

		//방문한 노드인지 찾아본다.
		//방문한 노드는 다시 방문하지 않도록 해준다.
		//우선순위 큐라 값을 찾지 못한다.
		bool visited = false;

		for (auto& a : _closelist)
		{
			if (newNode->pos == a->pos)
			{
				visited = true;
				break;
			}
		}
		if (visited)
		{
			delete newNode;
			continue;
		}

		//openlist에도 이미 방문중인 노드일 수 있음.
		visited = false;
		for (auto& a : _openlist)
		{
			if (newNode->pos == a->pos)
			{
				//만약 새로운 경로의 F가 더 적다면 갱신해준다.
				if (newNode->F < a->F)
				{
					a->G = newNode->G;
					a->H = newNode->H;
					a->F = newNode->F;
					a->parent = newNode->parent;
				}
				visited = true;
				break;
			}
		}
		if (visited)
		{
			delete newNode;
			continue;
		}



		_openlist.insert(newNode);
	}
}

void AStar::makeEmptyList()
{
	for (auto n : _openlist) delete n;
	for (auto n : _closelist) delete n;

	_openlist.clear();
	_closelist.clear();
	_shortestRoutelist.clear();
}

void AStar::updateNode()
{
	_startNode.pos.x = _start.x;
	_startNode.pos.y = _start.y;
}

bool AStar::findPath()
{
	makeEmptyList();

	insertListEightDirection(&_startNode);

	while (!_openlist.empty())
	{
		auto bestIt = _openlist.begin();
		Node* top = *bestIt;
		_openlist.erase(bestIt);
		//갔던 곳 다시 가지 않도록 표시
		_closelist.push_back(top);


		//현재 방문한 노드 출력
		//cout << "[x pos] : " << top->pos.x << " [y pos] : " << top->pos.y << "\n";
		//cout << " [G] : " << top->G << " [H] : " << top->H << " [F] : " << top->F << "\n";
		//최종 목적지에 도달했다면 중단
		if (top->pos == _destination)
		{
			//cout << "--------------최단 거리 경로 출력------------------------------" << "\n";
			//cout << "목적지에 도달했습니다." << "\n";
			//실제 최단거리를 꺼내 벡터에 담고 gdi에서 해당 자료구조를 순회하도록 한다.
			Node copy = *top;
			while (!(copy.pos == _startNode.pos))
			{
				//cout << "[x pos] : " << copy.pos.x << " [y pos] : " << copy.pos.y << "\n";
				_shortestRoutelist.push_back(copy.parent);
				copy = *copy.parent;
			}
			return true;
		}
		insertListEightDirection(top);
	}
	return false;
}

//일단 제일 처음 좌표에서 도착지까지 H는 계산되야 함.
float AStar::findHbyGrid(Grid* s)
{
	return abs(s->x - _destination.x) + abs(s->y - _destination.y);
}
