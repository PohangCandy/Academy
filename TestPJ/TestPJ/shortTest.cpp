#include "stdafx.h"
#include <unordered_map>

class Player
{
public:
	int id = 0;
};

std::unordered_map<int, Player*> uMapTest;

int main() {
	Player* p1 = new Player;
	p1->id = 0;
	Player* p2 = new Player;
	p2->id = 1;
	Player* p3 = new Player;
	p3->id = 2;
	uMapTest[0] = p1;
	uMapTest[1] = p2;
	uMapTest[2] = p3;

	auto a = uMapTest.find(p1->id);
	if (a != uMapTest.end())
	{
		uMapTest.erase(a);
	}


}