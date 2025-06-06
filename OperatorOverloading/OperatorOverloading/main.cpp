#include <iostream>
#include "my_new.h"
//매크로를 헤더에 포함시켰다.
//#define new new( __FILE__ , __LINE__)
//#define delete(a) delete((void*)a)

class MyClass
{
public:
	MyClass();
	~MyClass();

private:

};

MyClass::MyClass()
{
	printf("생성\n");
}

MyClass::~MyClass()
{
	printf("소멸\n");
}



int main()
{
	//MyClass* pc = new MyClass[10];
	//delete pc;

	//MyClass* pc = new MyClass;
	//delete[] pc;

	int* pi = new int;
	delete pi;
	delete pi;
	delete pi;

	return 0;
}