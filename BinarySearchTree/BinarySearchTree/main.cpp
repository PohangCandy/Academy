#include <iostream>
#include <vector>
#include <queue>
#include <algorithm>
#include "BinarySearchTree.h"
using namespace std;

//트리 검증을 위한 프로그램
//여러가지 랜덤한 데이터를 트리에 넣고 트리에 있는 결과와 원본 데이터 비교
//데이터 종류
//1.언밸런싱 -> 한쪽으로 쏠린 형태
// 계속 이전 데이터보다 큰 데이터 삽입하거나
// 이전 데이터보다 작은 데이터 삽입
// 오름차순, 내림 차순 으로 정렬 후 삽입
// 
//2.밸런싱 -> 완전 이진 트리 형태
// 등비 수열공식에 따라 완전 이진 트리 총 노드의 개수는 2^n - 1개의 데이터 
// 데이터 정렬 한 후
// 중앙에 있는 2^(n-1)번 째 항 부터 -1,+1한 인덱스 번호를 큐에 담아 front를 출력해가면서 해당 원소를 트리에 삽입 
// 
//3.랜덤
//걍 데이터 랜덤하게 생성한 후 삽입
//원본데이터 정렬 결과를 트리와 비교

class TestTree {

public:
	TestTree(int size)
	{
		_size = size;
	}

	//한쪽으로 쏠린 형태의 데이터
	void makeUnBalancing()
	{
		//랜덤 데이터로 리스트 채움
		makeListWithRand(_size, v_data);

		//내림차순이나 오름차순 랜덤하게 정렬
		int i = rand() % 2;
		switch (i)
		{
		case 0:
		{
			//내림차순
			sort(v_data.begin(), v_data.end(), [](const int& i1, const int& i2)
				{
					return i1 < i2;
				});
			break;
		}
		case 1:
		{
			//오름차순
			sort(v_data.begin(), v_data.end(), [](const int& i1, const int& i2)
				{
					return i1 > i2;
				});
			break;
		}
		}

		cout << "원본 데이터 : ";
		printfOriginalData();
	}

	void makePerfectBinaryTree()
	{
		//포화이진트리의 노드의 개수는 2^n - 1개 단위여야 함.
		//16~128 - 1을 우선 기준으로 잡아준다.
		int s = 1 << max(4, rand() % 8);
		s -= 1;
		//7
		//3

		vector<int> temp;
		makeListWithRand(s,temp);

		//이제 정렬한 후
		sort(temp.begin(), temp.end(), [](const int& i1, const int& i2)
			{
				return i1 > i2;
			});

		struct tag_temp {
			int data;
			int index;
			int s;
			int e;
		};


		//완전 이진트리가 되도록 중앙에서부터 원소를 넣어준다.
		queue<tag_temp> q;
		tag_temp tt;
		int middle = s / 2;
		tt.data = temp[middle];
		tt.index = middle;
		tt.s = 0;
		tt.e = s - 1;


		q.push(tt);
		//middle = 0,middle - 1, middle + 1,s
		//왼쪽의 끝 지점 s는 s/2가 되고, 우측은 그대로 s
		//왼쪽의 시작 지점은 그대로 이전돠 같은 지점이 되고, 우측은 middle + 1
		while (!q.empty())
		{
			//뻑킹 왜 무한 반복됨???------------------------------------------------------------
			tt = q.front();
			int f = tt.data;
			v_data.push_back(f);
			q.pop();
			//처음과 끝 지점을 제대로 넘겨줘야 함.
			tag_temp left, right;
			if (tt.index >= 0)
			{
				left.index = (tt.s + tt.index - 1) / 2;
				left.s = tt.s;
				left.e = tt.index - 1;
				left.data = temp[left.index];
				q.push(left);
			}

			if (tt.index < s)
			{
				right.index = (tt.e + tt.index + 1) / 2;
				right.s = tt.index + 1;
				right.e = tt.e;
				right.data = temp[right.index];
				q.push(right);
			}
		}

	}

	void makeListWithRand(int s, vector<int>& v)
	{
		v.clear();
		int radnum = rand() % 100;
		while (!checkDuplicate(radnum) && s > 0)
		{
			v.push_back(radnum);
			radnum = rand() % 100;
			s--;
		}
	}

	bool checkDuplicate(int d)
	{
		for (auto& a : v_data)
		{
			if (a == d) return true;
		}

		return false;
	}

	void InsertTree(BT* bt)
	{
		for (auto& a : v_data)
		{
			bt->Insert(a);
		}
	}

	void compareData(BT* bt)
	{
		//오름차순으로 원본 데이터 정렬
		cout << "원본 데이터 정렬 후: ";
		sort(v_data.begin(), v_data.end(), [](const int& i1, const int& i2)
			{
				return i1 > i2;
			});
		printfOriginalData();
		cout << "트리 중위 순회 결과 : ";
		bt->InOrder();
	}

	void printfOriginalData()
	{
		for (auto& a : v_data)
		{
			cout << a << " ";
		}
		cout << "\n";
	}

private:
	//삽입 삭제 테스트 하려면 리스트가 나을것으로 보임.
	vector <int> v_data;
	int _size;
};

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