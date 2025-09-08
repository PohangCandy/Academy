#include <iostream>
#include <list>
#include <set>
using namespace std;

#define width 100
#define length 100

int map[length][width];

struct Grid {
	float x;
	float y;

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

	AStar(Grid start, Grid destination) {

		_start = start;
		_destination = destination;

		_startNode.pos = start;
		_startNode.parent = nullptr;
		_startNode.G = 0;
		_startNode.H = findHbyGrid(&_startNode.pos);
		
		insertListEightDirection(&_startNode);
	}

	~AStar()
	{
		for (auto n : _openlist) delete n;
		for (auto n : _closelist) delete n;
	}

	bool findPath();

private:
	//방문해야 할 리스트
	//우선 순위 큐로 했더니 openlist에 이미 방문한 노드가 있을 경우 탐색을 할 수 없음.
	//1. 안정성을 위해 먼저 set으로 
	set <Node*, CompareNode>_openlist;
	list <Node*>_closelist;
	Node _startNode;
	Grid _start;
	Grid _destination;

	float findHbyGrid(Grid* s);
	float findGbyNode(Node* s, Node* d);
	float findF(Node* n);

	void insertListEightDirection(Node* startNode);
};

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
		if (nx < 0 || ny < 0 || nx >= width || ny >= length) continue;

		Node* newNode = new Node;
		newNode->parent = sn;
		newNode->pos.x = nx;
		newNode->pos.y = ny;
		newNode->G = findGbyNode(sn, newNode);
		newNode->H = findHbyGrid(&newNode->pos);
		newNode->F = findF(newNode);

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

bool AStar::findPath()
{
	while (!_openlist.empty())
	{
		auto bestIt = _openlist.begin();
		Node* top = *bestIt;
		_openlist.erase(bestIt);
		//갔던 곳 다시 가지 않도록 표시
		_closelist.push_back(top);

		//현재 방문한 노드 출력
		cout << "[x pos] : " << top->pos.x << " [y pos] : " << top->pos.y << "\n";
		cout << " [G] : " << top->G << " [H] : " << top->H  << " [F] : " << top->F << "\n";
		//최종 목적지에 도달했다면 중단
		if (top->pos == _destination)
		{ 
			cout << "목적지에 도달했습니다." << "\n";
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


int main()
{
	Grid S = { 0,0 };
	Grid D = { 10,0 };
	AStar as(S,D);

	if (!as.findPath())
	{
		cout << "목적지까지 가는 길이 없습니다!" << "\n";
	}

	return 0;
}