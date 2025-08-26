#include "MemoryPool.h"
#include <iostream>
using namespace std;

class A {
public:
	A() {}
	~A() { cout << "~A" << "\n"; }
	int a;
	int b;
};

class B {
public:
	B(){}
	~B() { cout << "~B" << "\n"; }
	int a;
	int b;
	int c;
};

int main()
{
	procademy::CMemoryPool<A> APool(10,true);
	procademy::CMemoryPool<B> BPool(10, true);

	A* p = APool.Alloc();
	B* pb = (B*)p;
	BPool.Free(pb);

	return 0;
}