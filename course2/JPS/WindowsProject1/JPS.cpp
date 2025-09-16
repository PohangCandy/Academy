#include "JPS.h"
#include <iostream>
using namespace std;

//일단 제일 처음 좌표에서 도착지까지 H는 계산되야 함.
//뉴클리드 계산
float JPS::findGbyNode(Node* s, Node* d)
{
	//대각선으로 이동한 경우
	if (abs(s->pos.x - d->pos.x) + abs(s->pos.y - d->pos.y) == 2) return s->G + 1.5;
	//직선으로 이동한 경우
	else return s->G + 1;
}

float JPS::findF(Node* n)
{
	return n->H + n->G;
}

//도착지가 맴버로 저장되어있으므로 출발지만 갱신하면서 재귀해주면 될 것으로 보임.
//탐색한 경로를 리스트에 담으면서 f가 가장 적은 곳을 먼저 탐색하도록 만든다.
void JPS::insertListEightDirection(Node* sn)
{
	setDirectionToTravel(sn, UU);
	setDirectionToTravel(sn, RU);
	setDirectionToTravel(sn, RR);
	setDirectionToTravel(sn, RD);
	setDirectionToTravel(sn, DD);
	setDirectionToTravel(sn, LD);
	setDirectionToTravel(sn, LL);
	setDirectionToTravel(sn, LU);
}

void JPS::makeInitList()
{
	for (auto n : _openlist) delete n;
	
	for (auto n : _closelist)
	{
		if (n == _startNode) continue;
		delete n;
	}

	_openlist.clear();
	_closelist.clear();
	_shortestRoutelist.clear();
}

void JPS::updateNode()
{
	_startNode->pos = _map->getStart();
	_goalNode->pos = _map->getGoal();
}

bool JPS::checkNodeIsInMap(Node* n)
{
	if (n->pos.x < 0 || n->pos.y < 0 || n->pos.x >= _map->getwidth() || n->pos.y >= _map->getheight())
	{
		return false;
	}

	return true;
}

bool JPS::checkGridIsInMap(Grid pos)
{
	if (pos.x < 0 || pos.y < 0 || pos.x >= _map->getwidth() || pos.y >= _map->getheight())
	{
		return false;
	}

	return true;
}

bool JPS::findPath()
{
	makeInitList();

	_map->InitMap();

	//가장 처음 시작 노드를 시작지점으로 잡아준다.
	//_closelist.push_back(_startNode);

	insertListEightDirection(_startNode);

	while (!_openlist.empty())
	{
		auto bestIt = _openlist.begin();
		Node* top = *bestIt;
		_openlist.erase(bestIt);

		if (top->pos == _goalNode->pos)
		{
			Node copy = *top;
			while (!(copy.pos == _startNode->pos))
			{
				_shortestRoutelist.push_back(copy.parent);
				copy = *copy.parent;
			}
			return true;
		}
		findNodeWithDirection(top);
	}
	return false;
}

bool JPS::CheckDiagonal(int x, int y, int dx, int dy)
{
	// dx=-1: 왼쪽, dx=1: 오른쪽
	if (dy == 0)
	{
		// 아래 대각선
		if (y + 1 < _map->getheight() &&
			_map->IsObstacle(y + 1, x) &&
			!_map->IsObstacle(y + 1, x + dx))
			return true;

		// 위 대각선
		if (y - 1 >= 0 &&
			_map->IsObstacle(y - 1, x) &&
			!_map->IsObstacle(y - 1, x + dx))
			return true;
	}
	// dy = -1 : 위쪽, dy = 1 : 아래쪽
	else if (dx == 0)
	{
		//우측 대각선
		if (x + 1 < _map->getwidth() &&
			_map->IsObstacle(y, x + 1) &&
			!_map->IsObstacle(y + dy, x + 1))
			return true;
		//좌측 대각선
		if (x - 1 >= 0 &&
			_map->IsObstacle(y, x - 1) &&
			!_map->IsObstacle(y + dy, x - 1))
			return true;
	}
	//대각선의 경우
	else
	{
		//dy = 1, : RU, LU 
		if (dy == 1)
		{
			// 아래 대각선
			if (y + 1 < _map->getheight() &&
				_map->IsObstacle(y + 1, x) &&
				!_map->IsObstacle(y + 1, x + dx))
				return true;
		}
		//dy = -1, : RD, LD 
		else
		{
			// 아래 대각선
			if (y + 1 < _map->getheight() &&
				_map->IsObstacle(y + 1, x) &&
				!_map->IsObstacle(y + 1, x + dx))
				return true;
		}

		//dx = 1, : RU, RD 
		if (dx == 1)
		{
			//좌측 대각선
			if (x - 1 >= 0 &&
				_map->IsObstacle(y, x - 1) &&
				!_map->IsObstacle(y + dy, x - 1))
				return true;
		}
		//dx = -1, : RD, LD 
		else
		{
			//우측 대각선
			if (x + 1 < _map->getwidth() &&
				_map->IsObstacle(y, x + 1) &&
				!_map->IsObstacle(y + dy, x + 1))
				return true;
		}

	}
	return false;
}

void JPS::ExploreDirection(Node* node, int dx, int dy)
{
	Node* temp = new Node;
	temp->pos = node->pos;
	temp->G = node->G;
	temp->H = node->H;
	temp->parent = node->parent;

	while (true)
	{
		bool dIsDiagonal = false;
		if (abs(dx) + abs(dy) == 2) dIsDiagonal = true;

		int newX = temp->pos.x + dx;
		int newY = temp->pos.y + dy;

		// 맵 범위 체크
		if (newX < 0 || newX >= _map->getwidth() ||
			newY < 0 || newY >= _map->getheight())
			break;

		// 장애물이면 종료
		if (_map->IsObstacle(newY, newX))
			break;

		// 이동
		temp->pos.x = newX;
		temp->pos.y = newY;
		if (dIsDiagonal)
		{
			temp->G += 1.4;
		}
		else
		{
			temp->G += 1;
		}
		temp->H = findHbyGrid(&temp->pos);
		temp->F = findF(temp);

		int x = temp->pos.x;
		int y = temp->pos.y;

		//대각선은 여기서 직선을 한번 더 탐사
		if (dIsDiagonal)
		{
			//RR
			if (dx == 1)
			{
				ExploreDirection(temp, 1, 0);
			}
			//LL
			else
			{
				ExploreDirection(temp, -1, 0);
			}
			//UU
			if (dy == 1)
			{
				ExploreDirection(temp, 0, 1);
			}
			//DD
			else
			{
				ExploreDirection(temp, 0, -1);
			}
		}

		// --- 양옆 대각선 검사 ---
		// (예: LL일 때 아래/위 왼쪽, RR일 때 아래/위 오른쪽)
		if (CheckDiagonal(x, y, dx, dy))
		{
			Node* n = new Node(*temp);
			_openlist.insert(n);
			_map->ChangeTile(y, x, nodelist);
			break;
		}

		// 목표 검사
		if (x == _goalNode->pos.x && y == _goalNode->pos.y)
		{
			Node* n = new Node(*temp);
			_openlist.insert(n);
			break;
		}

		// 방문 마킹
		_map->ChangeTile(y, x, visited);
	}

	delete temp;
	temp = nullptr;
}

void JPS::setDirectionToTravel(Node* n, EDirection d)
{
	//해당 노드가 탐사 가능한 노드인지 탐색하는 작업을 이 함수에서 하겠다.


	Node* temp = new Node;
	temp->pos  = n->pos;
	temp->G = n->G;
	temp->H = n->H;
	temp->parent = n;

	switch (d)
	{
	case LL:
	{
		ExploreDirection(temp, -1, 0);
	}
	break;
	case LU:
	{
		ExploreDirection(temp, -1, -1);
	}
		break;
	case UU:
	{
		ExploreDirection(temp, 0, -1);
	}
		break;
	case RU:
	{
		ExploreDirection(temp, 1, -1);
	}
		break;
	case RR:
	{
		ExploreDirection(temp, 1, 0);
	}
		break;
	case RD:
	{
		ExploreDirection(temp, 1, 1);
	}
		break;
	case DD:
	{
		ExploreDirection(temp, 0, 1);
	}
		break;
	case LD:
	{
		ExploreDirection(temp, -1, 1);
	}
		break;
	default:
		break;
	}

	delete temp;
	temp = nullptr;
}

void JPS::findNodeWithDirection(Node* n)
{
	EDirection d = n->getNodedirection();
	switch (d)
	{
	case LL:
	{
		setDirectionToTravel(n, LL);
		//대각선 방향 탐사 여부 
		//n의 DD가 장애물 + LD가 빈 공간인 경우 LD 방향 탐사
		if (_map->IsObstacle(n->pos.y + 1, n->pos.x) && !(_map->IsObstacle(n->pos.y + 1, n->pos.x - 1)))
		{
			setDirectionToTravel(n, LD);
		}
		//n의 UU가 장애물 + LU가 빈 공간이 경우 LU 방향 탐사
		if (_map->IsObstacle(n->pos.y - 1, n->pos.x) && !(_map->IsObstacle(n->pos.y - 1, n->pos.x - 1)))
		{
			setDirectionToTravel(n, LU);
		}
	}
		break;
	case LU:
	{
		setDirectionToTravel(n, LU);
		setDirectionToTravel(n, LL);
		setDirectionToTravel(n, UU);
		//대각선 방향 탐사 여부 
		//n의 DD가 장애물 + LD가 빈 공간인 경우 LD 방향 탐사
		if (_map->IsObstacle(n->pos.y + 1, n->pos.x) && !(_map->IsObstacle(n->pos.y + 1, n->pos.x - 1)))
		{
			setDirectionToTravel(n, LD);
		}
		//n의 RR가 장애물 + RU가 빈 공간이 경우 RU 방향 탐사
		if (_map->IsObstacle(n->pos.y, n->pos.x + 1) && !(_map->IsObstacle(n->pos.y - 1, n->pos.x + 1)))
		{
			setDirectionToTravel(n, RU);
		}
	}
		break;
	case UU:
	{
		setDirectionToTravel(n, UU);
		//대각선 방향 탐사 여부 
		//n의 LL가 장애물 + LU가 빈 공간인 경우 LU 방향 탐사
		if (_map->IsObstacle(n->pos.y, n->pos.x - 1) && !(_map->IsObstacle(n->pos.y - 1, n->pos.x - 1)))
		{
			setDirectionToTravel(n, LU);
		}
		//n의 RR가 장애물 + RU가 빈 공간이 경우 RU 방향 탐사
		if (_map->IsObstacle(n->pos.y, n->pos.x + 1) && !(_map->IsObstacle(n->pos.y - 1, n->pos.x + 1)))
		{
			setDirectionToTravel(n, RU);
		}
	}
		break;
	case RU:
	{
		setDirectionToTravel(n, RU);
		setDirectionToTravel(n, RR);
		setDirectionToTravel(n, UU);
		//대각선 방향 탐사 여부 
		//n의 DD가 장애물 + RD가 빈 공간인 경우 RD 방향 탐사
		if (_map->IsObstacle(n->pos.y + 1, n->pos.x) && !(_map->IsObstacle(n->pos.y + 1, n->pos.x + 1)))
		{
			setDirectionToTravel(n, RD);
		}
		//n의 LL가 장애물 + LU가 빈 공간인 경우 LU 방향 탐사
		if (_map->IsObstacle(n->pos.y, n->pos.x - 1) && !(_map->IsObstacle(n->pos.y - 1, n->pos.x - 1)))
		{
			setDirectionToTravel(n, LU);
		}
	}
		break;
	case RR: 
	{
		setDirectionToTravel(n, RR);
		//대각선 방향 탐사 여부 
		//n의 DD가 장애물 + RD가 빈 공간인 경우 RD 방향 탐사
		if (_map->IsObstacle(n->pos.y + 1, n->pos.x) && !(_map->IsObstacle(n->pos.y + 1, n->pos.x + 1)))
		{
			setDirectionToTravel(n, RD);
		}
		//n의 UU가 장애물 + RU가 빈 공간이 경우 RU 방향 탐사
		if (_map->IsObstacle(n->pos.y - 1, n->pos.x) && !(_map->IsObstacle(n->pos.y - 1, n->pos.x + 1)))
		{
			setDirectionToTravel(n, RU);
		}
	}
		break;
	case RD:
	{
		setDirectionToTravel(n, RD);
		setDirectionToTravel(n, RR);
		setDirectionToTravel(n, DD);
		//n의 UU가 장애물 + RU가 빈 공간이 경우 RU 방향 탐사
		if (_map->IsObstacle(n->pos.y - 1, n->pos.x) && !(_map->IsObstacle(n->pos.y - 1, n->pos.x + 1)))
		{
			setDirectionToTravel(n, RU);
		}
		//n의 LL가 장애물 + LD가 빈 공간인 경우 LU 방향 탐사
		if (_map->IsObstacle(n->pos.y, n->pos.x - 1) && !(_map->IsObstacle(n->pos.y + 1, n->pos.x - 1)))
		{
			setDirectionToTravel(n, LD);
		}
	}
		break;
	case DD:
	{
		setDirectionToTravel(n, DD);
		//대각선 방향 탐사 여부 
		//n의 LL가 장애물 + LD가 빈 공간인 경우 LU 방향 탐사
		if (_map->IsObstacle(n->pos.y, n->pos.x - 1) && !(_map->IsObstacle(n->pos.y + 1, n->pos.x - 1)))
		{
			setDirectionToTravel(n, LD);
		}
		//n의 RR가 장애물 + RU가 빈 공간이 경우 RU 방향 탐사
		if (_map->IsObstacle(n->pos.y, n->pos.x + 1) && !(_map->IsObstacle(n->pos.y + 1, n->pos.x + 1)))
		{
			setDirectionToTravel(n, RD);
		}
	}
		break;
	case LD:
	{
		setDirectionToTravel(n, LD);
		setDirectionToTravel(n, LL);
		setDirectionToTravel(n, DD);
		//n의 LL가 장애물 + LU가 빈 공간인 경우 LU 방향 탐사
		if (_map->IsObstacle(n->pos.y, n->pos.x - 1) && !(_map->IsObstacle(n->pos.y - 1, n->pos.x - 1)))
		{
			setDirectionToTravel(n, LU);
		}
		//n의 RR가 장애물 + RU가 빈 공간이 경우 RU 방향 탐사
		if (_map->IsObstacle(n->pos.y, n->pos.x + 1) && !(_map->IsObstacle(n->pos.y + 1, n->pos.x + 1)))
		{
			setDirectionToTravel(n, RD);
		}
	}
		break;
	default:
		break;
	}
}

//일단 제일 처음 좌표에서 도착지까지 H는 계산되야 함.
float JPS::findHbyGrid(Grid* s)
{
	return abs(s->x - _goalNode->pos.x) + abs(s->y - _goalNode->pos.y);
}
