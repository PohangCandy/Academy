#include <iostream>
#include "my_new.h"
//매크로를 헤더에 포함시켰다.
//#define new new( __FILE__ , __LINE__)
//#define delete(a) delete((void*)a)

int main()
{
	int* pi = new int;

	delete[](pi);
	return 0;
}