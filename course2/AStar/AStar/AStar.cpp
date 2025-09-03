#include <iostream>
#include <queue>
using namespace std;

int map[100][100];

struct Grid {
	int x;
	int y;
};

struct Node {
	Grid pos;
	Node* parent;
	int G; //출발점으로부터의 이동 거리
	int H; //목적지까지의 거리(장애물을 신경쓰지 않은 직선 거리)
	int F; //G + H
};

class AStar {

public:

	AStar(Grid start, Grid destination) {
		_start = start;
		_destination = destination;
		findPath();
	}

	int findHbyGrid(Grid s);

private:

	priority_queue <Node>_openlist;
	priority_queue<Node>_closelist;
	Grid _start;
	Grid _destination;

	void findG();

	void findPath();
};

//좌표에 도착할때마다 매번 계산해야 하나?
//아니면 출발지의 좌표는 고정이고 알고있으니
//총 8개의 좌표에 대해 각각 +1 또는 +1.4를 해줘도 됨.
void AStar::findG()
{

}

//도착지가 맴버로 저장되어있으므로 출발지만 갱신하면서 재귀해주면 될 것으로 보임.
//탐색한 경로를 리스트에 담으면서 f가 가장 적은 곳을 먼저 탐색하도록 만든다.
void AStar::findPath()
{

}

//일단 제일 처음 좌표에서 도착지까지 H는 계산되야 함.
int AStar::findHbyGrid(Grid s)
{
	return abs(s.x - _destination.x) + abs(s.y - _destination.y);
}


int main()
{
	Grid S = { 0,0 };
	Grid D = { 0,0 };
	AStar as(S,D);

	cout << as.findHbyGrid(S);

	return 0;
}