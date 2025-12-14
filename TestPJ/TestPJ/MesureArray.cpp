#include "stdafx.h"

char array[10];

int main() {

	char* cparr = new char[10];

	std::cout << sizeof(array) << std::endl; //40
	std::cout << sizeof(*array) << std::endl; //4
	std::cout << sizeof(cparr) << std::endl; //8
}