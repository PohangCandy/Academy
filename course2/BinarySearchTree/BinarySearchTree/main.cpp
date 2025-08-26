#include <iostream>
#include "BinarySearchTree.h"
#include "TestTree.h"
using namespace std;



int main()
{

	//while (1)
	//{
	//	int s = 1 << max(4, rand() % 8);
	//	cout << s << "\n";
	//}

	BT bt;

	TestTree tt(10);
	//tt.makeUnBalancing();
	tt.makePerfectBinaryTree();
	tt.InsertTree(&bt);
	tt.compareData(&bt);

	//bool loop = true;
	//while (loop)
	//{
	//	cout << "번호를 입력하세요." << "\n";
	//	cout << "1 : 삽입 " << "\n";
	//	cout << "2 : 삭제" << "\n";
	//	cout << "3 : 종료" << "\n";

	//	int input;
	//	cin >> input;
	//	switch (input)
	//	{
	//	case 1:
	//	{
	//		cout << "삽입할 데이터를 입력하세요. : " << "\n";
	//		int data;
	//		cin >> data;
	//		bt.Insert(data);

	//		//자동으로 중위순회 결과 트리 출력
	//		bt.InOrder();
	//		cout << "\n";
	//		break;
	//	}
	//	case 2:
	//	{
	//		cout << "삭제할 데이터를 입력하세요. : " << "\n";
	//		int data;
	//		cin >> data;
	//		bt.Remove(data);

	//		//자동으로 중위순회 결과 트리 출력
	//		bt.InOrder();
	//		cout << "\n";
	//		break;
	//	}
	//	case 3:
	//	{
	//		cout << "프로그램을 종료합니다." << "\n";
	//		loop = false;
	//		break;
	//	}
	//	default:
	//	{
	//		cout << "잘못된 번호입니다." << "\n";
	//		cout << "다시 입력하세요." << "\n";
	//		break;
	//	}

	//	}
	//}


	return 0;
}