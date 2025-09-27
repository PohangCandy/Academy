#include "JPS.h"
#include <iostream>
using namespace std;

//일단 제일 처음 좌표에서 도착지까지 H는 계산되야 함.
//뉴클리드 계산
//float JPS::findGbyNode(Node* s, Node* d)
//{
//	//대각선으로 이동한 경우
//	if (abs(s->pos.x - d->pos.x) + abs(s->pos.y - d->pos.y) == 2) return s->G + 1.5;
//	//직선으로 이동한 경우
//	else return s->G + 1;
//}

//float JPS::findF(Node* n)
//{
//	return n->H + n->G;
//}

//도착지가 맴버로 저장되어있으므로 출발지만 갱신하면서 재귀해주면 될 것으로 보임.
//탐색한 경로를 리스트에 담으면서 f가 가장 적은 곳을 먼저 탐색하도록 만든다.
void JPS::insertListEightDirection()
{
	setDirectionToTravel(_startGrid, LL);
	setDirectionToTravel(_startGrid, LU);
	setDirectionToTravel(_startGrid, UU);
	setDirectionToTravel(_startGrid, RU);
	setDirectionToTravel(_startGrid, RR);
	setDirectionToTravel(_startGrid, RD);
	setDirectionToTravel(_startGrid, DD);
	setDirectionToTravel(_startGrid, LD);
}

void JPS::makeInitList()
{
	//for (auto n : _openlist) delete n;
	
	//for (auto n : _closelist)
	//{
	//	if (n == _startNode) continue;
	//	delete n;
	//}

	//for (auto n : _shortestRoutelist)
	//{
	//	if (n == _startNode) continue;
	//	if (n == _goalNode) continue;
	//}

	_openlist.clear();
	//_closelist.clear();
	//_shortestRoutelist.clear();
}

void JPS::updateNode()
{
	_startGrid = _map->getStart();
	_goalGrid = _map->getGoal();
}

//bool JPS::checkNodeIsInMap(Node* n)
//{
//	if (n->pos.x < 0 || n->pos.y < 0 || n->pos.x >= _map->getwidth() || n->pos.y >= _map->getheight())
//	{
//		return false;
//	}
//
//	return true;
//}

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
	//메모리 누수 테스트용
	//Node* n = new Node[100];

	makeInitList();

	_map->InitMap();

	//가장 처음 시작 노드를 시작지점으로 잡아준다.
	//_closelist.push_back(_startNode);

	_map->ChangeTile(_startGrid->y, _startGrid->x, nodelist);
	int sH = findHbyGrid(_startGrid->y, _startGrid->x);
	_map->setMapData(_startGrid->y, _startGrid->x, 0, sH, 0, nullptr);
	insertListEightDirection();

	while (!_openlist.empty())
	{
		auto bestIt = _openlist.begin();
		Grid* top = _map->getGrid((*bestIt)->y, (*bestIt)->x);

		if (top == nullptr)
		{
			cout << "w";
			//잡있다ㅣ 요놈
		}
		//그리드 속성이 더 정확해보이는데?
		//그리드의 부모로 찾아준다.
		//그리고 리스트에서 뽑아낸 친구는 바로 제거하자.
		if (top == _goalGrid)
		{
			//_openlist.erase(bestIt);
			//Node copy = *top;
			while (!(top == _startGrid))
			{
				if (top->gparent == nullptr)
				{
					cout << "w";
					//잡있다ㅣ 요놈
				}
				if (top->gparent == _startGrid)
				{

				}
				else
				{
					//top->parent->pos.type = shortest;
					//_shortestRoutelist.push_back(top->parent);
					_map->ChangeTile(top->gparent->y, top->gparent->x, shortest);
				}
				top = top->gparent;
				if (top == nullptr)
				{
					cout << "w";
					//잡있다ㅣ 요놈
				}
			}
			return true;
		}
		else
		{
			findNodeWithDirection(*bestIt);
		}

		//해제가 일어나야 하지 않나?
		_openlist.erase(bestIt);
		//delete* bestIt;
		//(*bestIt) = nullptr;
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

		//dy = 1, : Down 방향으로 진행중
		// 위칸 방해물 체크 후 대각선 방향 진행 결정
		if (dy == 1)
		{
			if (y + 1 < _map->getheight() &&
				_map->IsObstacle(y - 1, x) &&
				!_map->IsObstacle(y - 1, x + dx))
				return true;
		}
		//dy = -1, : Up 방향으로 진행중
		// 아래칸 방해물 체크 후 대각선 방향 진행 결정
		else
		{
			if (y + 1 < _map->getheight() &&
				_map->IsObstacle(y + 1, x) &&
				!_map->IsObstacle(y + 1, x + dx))
				return true;
		}

		//dx = 1, : Right 방향으로 진행중
		//좌측 방해물 확인 후 진행
		if (dx == 1)
		{
			if (x - 1 >= 0 &&
				_map->IsObstacle(y, x - 1) &&
				!_map->IsObstacle(y + dy, x - 1))
				return true;
		}
		//dx = -1, : RD, LD 
		else
		{
			if (x + 1 < _map->getwidth() &&
				_map->IsObstacle(y, x + 1) &&
				!_map->IsObstacle(y + dy, x + 1))
				return true;
		}

	}
	return false;
}

//주어진 노드를 부모 노드로 설정하여, 
//주어진 방향으로 탐색하는 함수
bool JPS::ExploreDirection(Grid* g, int dx, int dy)
{
	bool findNode = false;

	//Node* temp = new Node;
	//temp->pos = node->pos;
	//temp->G = node->G;
	//temp->H = node->H;
	//temp->parent = node;
	/*temp->pos.gparent = &node->pos;*/

	int newX = g->x;
	int newY = g->y;
	float newH = g->h;
	float newG = g->g;
	float newF = g->f;

	while (true)
	{

		//좌표와 설정 값 계산
		newX = newX + dx;
		newY = newY + dy;

		//대각선 체크
		bool dIsDiagonal = false;
		if (abs(dx) + abs(dy) == 2) dIsDiagonal = true;

		if (dIsDiagonal)
		{
			newG += 1.5;
		}
		else
		{
			newG += 1;
		}

		newH = findHbyGrid(newY,newX);
		newF = newG + newH;


		//갈 수 없는 상황

		//1. 맵 범위 밖
		if (newX < 0 || newX >= _map->getwidth() ||
			newY < 0 || newY >= _map->getheight())
			break;

		//2. 장애물
		if (_map->IsObstacle(newY, newX))
			break;

		//이미 방문함 + F값이 더 적게 기록되어 있음.
		if (_map->CheckTile(newY, newX) == visited || _map->CheckTile(newY, newX) == nodelist)
		{
			Grid* curG = _map->getGrid(newY, newX);
			if( newF > curG->f ) break;
			//새로운 F값이 같거나 더 적은 경우 갱신
			//같은 값인 F값을 갱신시키도록 허락하는게 맞을까?
		}

		_map->ChangeTile(newY, newX, visited);
		_map->setMapData(newY, newX, newG, newH, newF, g);

		//대각선은 여기서 직선을 한번 더 탐사해야 함.
		if (dIsDiagonal)
		{
			Grid* temp = _map->getGrid(newY, newX);
			//RU, RD
			if (dx == 1)
			{
				if (ExploreDirection(temp, 1, 0)) break;
			}
			//LU, LD
			else
			{
				if (ExploreDirection(temp, -1, 0)) break;
			}
			//RD,LD
			if (dy == 1)
			{
				if (ExploreDirection(temp, 0, 1)) break;
			}
			//RU,LU
			else
			{
				if (ExploreDirection(temp, 0, -1)) break;
			}
		}

		// --- 양옆 대각선 검사 ---
		// (예: LL일 때 아래/위 왼쪽, RR일 때 아래/위 오른쪽)
		if (CheckDiagonal(newX, newY, dx, dy))
		{
			Grid* pG;
			if (_map->CheckTile(g->y, g->x) == nodelist)
			{
				pG = _map->getGrid(newY, newX);
			}
			else
			{
				pG = g;
			}
			_openlist.insert(pG);
			_map->ChangeTile(pG->y, pG->x, nodelist);
			findNode = true;
			break;
		}

		// 목표 검사
		//위와 또 같은 로직
		if (newX == _goalGrid->x && newY == _goalGrid->y)
		{
			Grid* pG;
			if (_map->CheckTile(g->y, g->x) == nodelist)
			{
				pG = _map->getGrid(newY, newX);
			}
			else
			{
				pG = g;
			}
			_openlist.insert(pG);
			_map->ChangeTile(pG->y, pG->x, nodelist);
			findNode = true;
			break;
		}


	}

	return findNode;
}

void JPS::setDirectionToTravel(Grid* g, EDirection d)
{
	//해당 노드가 탐사 가능한 노드인지 탐색하는 작업을 이 함수에서 하겠다.


	//Node* temp = new Node;
	//temp->pos  = n->pos;
	//temp->G = n->G;
	//temp->H = n->H;
	//temp->parent = n;

	switch (d)
	{
	case LL:
	{
		ExploreDirection(g, -1, 0);
	}
	break;
	case LU:
	{
		ExploreDirection(g, -1, -1);
	}
		break;
	case UU:
	{
		ExploreDirection(g, 0, -1);
	}
		break;
	case RU:
	{
		ExploreDirection(g, 1, -1);
	}
		break;
	case RR:
	{
		ExploreDirection(g, 1, 0);
	}
		break;
	case RD:
	{
		ExploreDirection(g, 1, 1);
	}
		break;
	case DD:
	{
		ExploreDirection(g, 0, 1);
	}
		break;
	case LD:
	{
		ExploreDirection(g, -1, 1);
	}
		break;
	default:
		break;
	}

	//delete temp;
	//temp = nullptr;
}

//리스트에 있는 노드 중 F값이 가장 작은 노드의 부모 노드 반대 방향으로 탐색 
void JPS::findNodeWithDirection(Grid* g)
{
	EDirection d = g->getNodedirection();
	switch (d)
	{
	case LL:
	{
		setDirectionToTravel(g, LL);
		//대각선 방향 탐사 여부 
		//n의 DD가 장애물 + LD가 빈 공간인 경우 LD 방향 탐사
		if (_map->IsObstacle(g->y + 1, g->x) && !(_map->IsObstacle(g->y + 1, g->x - 1)))
		{
			setDirectionToTravel(g, LD);
		}
		//n의 UU가 장애물 + LU가 빈 공간이 경우 LU 방향 탐사
		if (_map->IsObstacle(g->y - 1, g->x) && !(_map->IsObstacle(g->y - 1, g->x - 1)))
		{
			setDirectionToTravel(g, LU);
		}
	}
		break;
	case LU:
	{
		setDirectionToTravel(g, LU);
		setDirectionToTravel(g, LL);
		setDirectionToTravel(g, UU);
		//대각선 방향 탐사 여부 
		//n의 DD가 장애물 + LD가 빈 공간인 경우 LD 방향 탐사
		if (_map->IsObstacle(g->y + 1, g->x) && !(_map->IsObstacle(g->y + 1, g->x - 1)))
		{
			setDirectionToTravel(g, LD);
		}
		//n의 RR가 장애물 + RU가 빈 공간이 경우 RU 방향 탐사
		if (_map->IsObstacle(g->y, g->x + 1) && !(_map->IsObstacle(g->y - 1, g->x + 1)))
		{
			setDirectionToTravel(g, RU);
		}
	}
		break;
	case UU:
	{
		setDirectionToTravel(g, UU);
		//대각선 방향 탐사 여부 
		//n의 LL가 장애물 + LU가 빈 공간인 경우 LU 방향 탐사
		if (_map->IsObstacle(g->y, g->x - 1) && !(_map->IsObstacle(g->y - 1, g->x - 1)))
		{
			setDirectionToTravel(g, LU);
		}
		//n의 RR가 장애물 + RU가 빈 공간이 경우 RU 방향 탐사
		if (_map->IsObstacle(g->y, g->x + 1) && !(_map->IsObstacle(g->y - 1, g->x + 1)))
		{
			setDirectionToTravel(g, RU);
		}
	}
		break;
	case RU:
	{
		setDirectionToTravel(g, RU);
		setDirectionToTravel(g, RR);
		setDirectionToTravel(g, UU);
		//대각선 방향 탐사 여부 
		//n의 DD가 장애물 + RD가 빈 공간인 경우 RD 방향 탐사
		if (_map->IsObstacle(g->y + 1, g->x) && !(_map->IsObstacle(g->y + 1, g->x + 1)))
		{
			setDirectionToTravel(g, RD);
		}
		//n의 LL가 장애물 + LU가 빈 공간인 경우 LU 방향 탐사
		if (_map->IsObstacle(g->y, g->x - 1) && !(_map->IsObstacle(g->y - 1, g->x - 1)))
		{
			setDirectionToTravel(g, LU);
		}
	}
		break;
	case RR: 
	{
		setDirectionToTravel(g, RR);
		//대각선 방향 탐사 여부 
		//n의 DD가 장애물 + RD가 빈 공간인 경우 RD 방향 탐사
		if (_map->IsObstacle(g->y + 1, g->x) && !(_map->IsObstacle(g->y + 1, g->x + 1)))
		{
			setDirectionToTravel(g, RD);
		}
		//n의 UU가 장애물 + RU가 빈 공간이 경우 RU 방향 탐사
		if (_map->IsObstacle(g->y - 1, g->x) && !(_map->IsObstacle(g->y - 1, g->x + 1)))
		{
			setDirectionToTravel(g, RU);
		}
	}
		break;
	case RD:
	{
		setDirectionToTravel(g, RD);
		setDirectionToTravel(g, RR);
		setDirectionToTravel(g, DD);
		//n의 UU가 장애물 + RU가 빈 공간이 경우 RU 방향 탐사
		if (_map->IsObstacle(g->y - 1, g->x) && !(_map->IsObstacle(g->y - 1, g->x + 1)))
		{
			setDirectionToTravel(g, RU);
		}
		//n의 LL가 장애물 + LD가 빈 공간인 경우 LU 방향 탐사
		if (_map->IsObstacle(g->y, g->x - 1) && !(_map->IsObstacle(g->y + 1, g->x - 1)))
		{
			setDirectionToTravel(g, LD);
		}
	}
		break;
	case DD:
	{
		setDirectionToTravel(g, DD);
		//대각선 방향 탐사 여부 
		//n의 LL가 장애물 + LD가 빈 공간인 경우 LU 방향 탐사
		if (_map->IsObstacle(g->y, g->x - 1) && !(_map->IsObstacle(g->y + 1, g->x - 1)))
		{
			setDirectionToTravel(g, LD);
		}
		//n의 RR가 장애물 + RU가 빈 공간이 경우 RU 방향 탐사
		if (_map->IsObstacle(g->y, g->x + 1) && !(_map->IsObstacle(g->y + 1, g->x + 1)))
		{
			setDirectionToTravel(g, RD);
		}
	}
		break;
	case LD:
	{
		setDirectionToTravel(g, LD);
		setDirectionToTravel(g, LL);
		setDirectionToTravel(g, DD);
		//n의 UU가 장애물 + LU가 빈 공간인 경우 LU 방향 탐사
		if (_map->IsObstacle(g->y - 1, g->x) && !(_map->IsObstacle(g->y - 1, g->x - 1)))
		{
			setDirectionToTravel(g, LU);
		}
		//n의 RR가 장애물 + RU가 빈 공간이 경우 RU 방향 탐사
		if (_map->IsObstacle(g->y, g->x + 1) && !(_map->IsObstacle(g->y + 1, g->x + 1)))
		{
			setDirectionToTravel(g, RD);
		}
	}
		break;
	default:
		break;
	}
}

//일단 제일 처음 좌표에서 도착지까지 H는 계산되야 함.
float JPS::findHbyGrid(int y, int x)
{
	return abs(x - _goalGrid->x) + abs(y - _goalGrid->y);
}
