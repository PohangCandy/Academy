#include <iostream>
#include "TemplateLinkedList.h"

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

int main()
{
	TestBasicOperations();
	TestPointerList();

	return 0;
}