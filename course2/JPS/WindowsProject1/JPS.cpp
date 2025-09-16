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

	//int dx[8] = { 0,1,1,1,0,-1,-1,-1 };
	//int dy[8] = { 1,1,0,-1,-1,-1,0,1 };

	//for (int i = 0; i < 8; i++)
	//{
	//	int nx = sn->pos.x + dx[i];
	//	int ny = sn->pos.y + dy[i];
	//	if (nx < 0 || ny < 0 || nx >= _map->getwidth() || ny >= _map->getheight()) continue;
	//	//여기서 장애물이 있는 지역은 못가게 해야하지 않을까?
	//	//맵 인스턴스를 받아서 맵이 가진 장애물 체크 로직을 불러오게 하자.
	//	if (_map->IsObstacle(ny, nx)) continue;
	//	//그냥 그리드로 방문할 좌표만 넣어도 될 것 같은데?
	//	//어차피 노드 만나기 전까진  F값 개무시하고 계속 탐사할 거임.
	//	//진행 방향이 필요하므로 parent 정보가 있어야 할 것으로 보임.
	//	//성능을 높이려면 new를 적게 쓰기위해 그리드 방향을 이용하되 parent의 방향만 이용하는게 나아보임.
	//	//근데 일단 안전하게 먼저 노드 만들어서 추적하기 쉽게 진행.
	//	Node* newNode =  new Node;
	//	newNode->parent = sn;
	//	newNode->pos.x = nx;
	//	newNode->pos.y = ny;
	//	newNode->G = findGbyNode(sn, newNode);
	//	newNode->H = findHbyGrid(&newNode->pos);
	//	newNode->F = findF(newNode);
	//	findNodeWithDirection(newNode);
	//	
	//	//방문한 노드는 다시 방문하지 않도록 해준다.
	//	//우선순위 큐라 값을 찾지 못한다.
	//	//bool visited = false;
	//	
	//	//for (auto& a : _closelist)
	//	//{
	//	//	if (newNode->pos == a->pos)
	//	//	{
	//	//		//closedList는 이미 방문이 확정된 노드 즉, 가장 최솟값 F를 지난 것이므로 다시 갈 필요 없음.
	//	//		////만약 새로운 경로의 F가 더 적다면 갱신해준다.
	//	//		//if (newNode->F < a->F)
	//	//		//{
	//	//		//	a->G = newNode->G;
	//	//		//	a->H = newNode->H;
	//	//		//	a->F = newNode->F;
	//	//		//	a->parent = newNode->parent;
	//	//		//}
	//	//		visited = true;
	//	//		break;
	//	//	}
	//	//}
	//	//if (visited)
	//	//{
	//	//	delete newNode;
	//	//	continue;
	//	//}
	//	////openlist에도 이미 방문 후보인 노드
	//	//visited = false;
	//	//for (auto& a : _openlist)
	//	//{
	//	//	if (newNode->pos == a->pos)
	//	//	{
	//	//		//만약 새로운 경로의 F가 더 적다면 갱신해준다.
	//	//		if (newNode->F < a->F)
	//	//		{
	//	//			_openlist.insert(newNode);
	//	//			newNode = a;
	//	//		}
	//	//		visited = true;
	//	//		break;
	//	//	}
	//	//}
	//	//if (visited)
	//	//{
	//	//	_openlist.erase(newNode);
	//	//	delete newNode;
	//	//	continue;
	//	//}
	//	//_openlist.insert(newNode);
	//}
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
		//_openlist.erase(bestIt);
		//갔던 곳 다시 가지 않도록 표시
		//_closelist.push_back(top);



		//현재 방문한 노드 출력
		//cout << "[x pos] : " << top->pos.x << " [y pos] : " << top->pos.y << "\n";
		//cout << " [G] : " << top->G << " [H] : " << top->H << " [F] : " << top->F << "\n";
		//최종 목적지에 도달했다면 중단
		if (top->pos == _goalNode->pos)
		{
			_openlist.erase(bestIt);
			//cout << "--------------최단 거리 경로 출력------------------------------" << "\n";
			//cout << "목적지에 도달했습니다." << "\n";
			//실제 최단거리를 꺼내 벡터에 담고 gdi에서 해당 자료구조를 순회하도록 한다.
			Node copy = *top;
			while (!(copy.pos == _startNode->pos))
			{
				//cout << "[x pos] : " << copy.pos.x << " [y pos] : " << copy.pos.y << "\n";
				_shortestRoutelist.push_back(copy.parent);
				copy = *copy.parent;
			}
			return true;
		}
		findNodeWithDirection(top);
	}
	return false;
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
		//좌표가 맵 안에서 노드를 찾을때까지 해당 방향으로 계속 탐색
		//탐사했을때 아무것도 나오지 않은 공간은 검은색으로 나오도록 한다.
		while (temp->pos.x - 1 >= 0 && !_map->IsObstacle(temp->pos.y, temp->pos.x - 1))
		{
			temp->pos.x -= 1;
			temp->G += 1;
			temp->H = findHbyGrid(&temp->pos);
			temp->F = findF(temp);

			int x = temp->pos.x;
			int y = temp->pos.y;
			if (x - 1 >= 0 && y + 1 < _map->getheight())
			{
				//n의 DD가 장애물 + LD가 빈 공간인 경우 노드 생성
				if (_map->IsObstacle(y + 1,x) && !_map->IsObstacle(y + 1, x - 1))
				{
					_openlist.insert(temp);
					_map->ChangeTile(y, x, nodelist);
					break;
				}
			}

			if (x - 1 >= 0 && y - 1 >= 0)
			{
				//n의 UU가 장애물 + LU가 빈 공간이 경우 노드 생성
				if (_map->IsObstacle(y - 1, x) && !_map->IsObstacle(y - 1, x - 1))
				{
					_openlist.insert(temp);
					break;
				}
			}

			//목표를 만나면 목표를 집어넣고 반환한다.
			//목표물은 나중에 랜더하기전에 리스트에서 지워야 할 듯
			if (x == _goalNode->pos.x && y == _goalNode->pos.y)
			{
				_openlist.insert(temp);
				break;
			}

			//노드없이 탐사된 맵을 표시해준다.
			_map->ChangeTile(y, x, visited);
		}
	}

	break;
	case LU:
		break;
	case UU:
		break;
	case RU:
		break;
	case RR:
	{
		//좌표가 맵 안에서 노드를 찾을때까지 해당 방향으로 계속 탐색
		while (temp->pos.x + 1 < _map->getheight() && !_map->IsObstacle(temp->pos.y, temp->pos.x + 1))
		{
			temp->pos.x += 1;
			temp->G += 1;
			temp->H = findHbyGrid(&temp->pos);
			temp->F = findF(temp);

			int x = temp->pos.x;
			int y = temp->pos.y;
			if (x + 1 < _map->getwidth() && y + 1 < _map->getheight())
			{
				//n의 DD가 장애물 + RD가 빈 공간인 경우 노드 생성
				if (_map->IsObstacle(y + 1, x) && !_map->IsObstacle(y + 1, x + 1))
				{
					_openlist.insert(temp);
					_map->ChangeTile(y, x, nodelist);
					break;
				}
			}

			if (x + 1 < _map->getwidth() && y - 1 >= 0)
			{
				//n의 UU가 장애물 + RU가 빈 공간이 경우 노드 생성
				if (_map->IsObstacle(y - 1, x) && !_map->IsObstacle(y - 1, x + 1))
				{
					_openlist.insert(temp);
					_map->ChangeTile(y, x, nodelist);
					break;
				}
			}

			//목표를 만나면 목표를 집어넣고 반환한다.
			//목표물은 나중에 랜더하기전에 리스트에서 지워야 할 듯
			if (x == _goalNode->pos.x && y == _goalNode->pos.y)
			{
				_openlist.insert(temp);
				break;
			}

			//노드없이 탐사된 맵을 표시해준다.
			_map->ChangeTile(y, x, visited);
		}
	}
		break;
	case RD:
		break;
	case DD:
		break;
	case LD:
		break;
	default:
		break;
	}
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
		break;
	case UU:

		break;
	case RU:
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
		break;
	case DD:

		break;
	case LD:
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
