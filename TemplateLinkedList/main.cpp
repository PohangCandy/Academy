#include <iostream>
#include "TemplateLinkedList.h"

#include <list>
using namespace std;
void testSTL();
void testmine();

void TestBasicOperations();
void TestPointerList();

int main()
{
	//TestBasicOperations();
	//TestPointerList();
	//testSTL();
	testmine();
	return 0;
}

void testmine()
{
	CList<int> ml;
	CList<int>::iterator it1;
	CList<int>::iterator it2;

	ml.push_back(1);
	it1 = ml.begin();
	printf("%d\n", *it1);
	ml.pop_back();
	//stl은 해제된 메모리가 아닌 원래 메모리 나타내는데
	//지금은 해제된 메모리 나타냄.
	printf("%d\n", *it1);
}

void testSTL()
{
	list<int> my_list = { 10,20,30,40 };

	auto it = my_list.begin();
	my_list.pop_front();
	cout << *it;
}


void TestBasicOperations()
{
	CList<int> list;
	list.push_back(10);
	list.push_back(20);
	list.push_back(30);
	list.push_front(5);

	std::cout << "Original list: ";
	for (auto it = list.begin(); it != list.end(); ++it)
		std::cout << *it << " ";
	std::cout << "\n";

	list.pop_front(); // 5 제거
	list.pop_back();  // 30 제거

	std::cout << "After pop: ";
	for (auto it = list.begin(); it != list.end(); ++it)
		std::cout << *it << " ";
	std::cout << "\n";

	list.remove(20); // remove test

	std::cout << "After remove(20): ";
	for (auto it = list.begin(); it != list.end(); ++it)
		std::cout << *it << " ";
	std::cout << "\n";

	list.clear();
	std::cout << "After clear: size = " << list.size() << ", empty = " << list.empty() << "\n";
}

void TestPointerList()
{
	struct Dummy {
		int id;
		Dummy(int id) : id(id) {}
	};

	CList<Dummy*> list;
	list.push_back(new Dummy(1));
	list.push_back(new Dummy(2));
	list.push_back(new Dummy(3));

	for (auto iter = list.begin(); iter != list.end(); )
	{
		if ((*iter)->id == 2)
		{
			delete* iter; // 실제 객체 먼저 삭제
			iter = list.erase(iter); // 노드 삭제
		}
		else
		{
			++iter;
		}
	}

	std::cout << "Remaining IDs: ";
	for (auto iter = list.begin(); iter != list.end(); ++iter)
		std::cout << (*iter)->id << " ";
	std::cout << "\n";

	for (auto iter = list.begin(); iter != list.end(); ++iter)
		delete* iter;

	list.clear();
}
