#include "AStar.h"
#include <iostream>
using namespace std;

int main()
{
	Grid S = { 0,0 };
	Grid D = { 10,0 };
	AStar as(S, D);

	if (!as.findPath())
	{
		cout << "목적지까지 가는 길이 없습니다!" << "\n";
	}

	return 0;
}