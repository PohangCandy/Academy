#include <iostream>
#include "CTest.h"
using namespace std;

int main()
{

	for (int i = 0; i < 10; i++)
	{
		addTestArray();
	}

	extern Test t[3];
	t[1].a = 100;

	return 0;
}