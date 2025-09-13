#include "AStar.h"
#include "Dungeon.h"
#include <iostream>
using namespace std;

int main()
{
	Dungeon d;
	Grid S = { 0,0 };
	Grid D = { 6,2 };
	AStar as(S, D,&d);

	if (!as.findPath())
	{
		cout << "목적지까지 가는 길이 없습니다!" << "\n";
	}

	return 0;
}