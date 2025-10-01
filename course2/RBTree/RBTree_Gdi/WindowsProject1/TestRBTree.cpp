#include "TestRBTree.h"
#include <iostream>
#include <queue>
#include <algorithm>
using namespace std;

void TestTree::makeUnBalancing()
{
	//랜덤 데이터로 리스트 채움
	makeListWithRand(_size, v_data);

	//내림차순이나 오름차순 랜덤하게 정렬
	int i = rand() % 2;
	switch (i)
	{
	case 0:
	{
		//오름차순
		sort(v_data.begin(), v_data.end(), [](const int& i1, const int& i2)
			{
				return i1 < i2;
			});
		break;
	}
	case 1:
	{
		//내림차순
		sort(v_data.begin(), v_data.end(), [](const int& i1, const int& i2)
			{
				return i1 > i2;
			});
		break;
	}
	}
}

void TestTree::makePerfectBinaryTree()
{
	//포화이진트리의 노드의 개수는 2^n - 1개 단위여야 함.
	//16~128 - 1을 우선 기준으로 잡아준다.
	int s = 1 << max(4, rand() % 8);
	s -= 1;
	//7
	//3

	vector<int> vtemp;
	makeListWithRand(s, vtemp);

	//이제 정렬한 후
	sort(vtemp.begin(), vtemp.end());

	struct tag_length {
		int s;
		int e;
	};


	//완전 이진트리가 되도록 중앙에서부터 원소를 넣어준다.
	queue<tag_length> q;
	tag_length tt;
	tt.s = 0;
	tt.e = s - 1;


	q.push(tt);
	//middle = 0,middle - 1, middle + 1,s
	//왼쪽의 끝 지점 s는 s/2가 되고, 우측은 그대로 s
	//왼쪽의 시작 지점은 그대로 이전돠 같은 지점이 되고, 우측은 middle + 1
	while (!q.empty())
	{
		tt = q.front();
		//중앙값을 빼서 벡터에 더해준다.
		int index = (tt.e + tt.s) / 2;
		int data = vtemp[index];
		v_data.push_back(data);
		q.pop();


		//이후 좌우변의 시작지점과 끝지점을 넘겨준다.
		tag_length left, right;
		if (tt.s <= index - 1)
		{
			left.s = tt.s;
			left.e = index - 1;
			q.push(left);
		}

		if (index + 1 <= tt.e)
		{
			right.s = index + 1;
			right.e = tt.e;
			q.push(right);
		}
	}
}

void TestTree::InsertFullData(RBTree* rbt)
{
	for (auto& a : v_data)
	{
		rbt->Insert(a);
	}
}

void TestTree::RemoveAllData(RBTree* rbt)
{
	for (auto& a : v_data)
	{
		rbt->Remove(a);
	}
}

void TestTree::RemoveData(RBTree* rbt, int data)
{
	for (auto& a : v_data)
	{
		if(a == data)
		{
			rbt->Remove(a);
		}
	}
}

void TestTree::compareData(RBTree* rbt)
{
	//오름차순으로 원본 데이터 정렬
	cout << "원본 데이터 정렬 후: ";

	//실제 원본데이터를 변경시킬게 아니라 복사본으로 정렬한 데이터와 비교해야 될 듯
	vector<int> v_temp = v_data;
	sort(v_temp.begin(), v_temp.end());
	printfVData(v_temp);
	cout << "\n";
	cout << "트리 중위 순회 결과 : ";

	vector<int> InOrderResult;
	rbt->InOrder(InOrderResult);
	cout << "\n";
	if (InOrderResult == v_temp) {
		cout << "일치함!";
	}
	else
	{
		cout << "불일치~~";
	}
}

void TestTree::printfVData(vector<int> v)
{
	for (auto& a : v)
	{
		cout << a << " ";
	}
}

// TestRBTree.cpp

void TestTree::makeRandomCase()
{
	// 1. 데이터 크기 랜덤 설정 (예: 50~150개)
	_size = 50 + (rand() % 101); // 50부터 150까지 랜덤 크기

	// 2. 어떤 유형의 데이터를 만들지 랜덤으로 선택
	int caseType = rand() % 3; // 0: UnBalancing, 1: Perfect, 2: Rand

	// v_data를 초기화합니다.
	v_data.clear();

	// makeListWithRand의 중복 방지 연산 과부하를 막기 위해 범위 늘림
	// makeListWithRand(int s, vector<int>& v) 함수를 수정해야 함.

	switch (caseType)
	{
	case 0:
	{
		cout << "--- Case 0: UnBalancing Test (" << _size << " nodes) ---\n";
		makeUnBalancing(); // UnBalancing 케이스는 내부적으로 makeListWithRand 호출
		break;
	}
	case 1:
	{
		// PerfectBinaryTree는 자체적으로 size를 2^n - 1로 재설정하므로,
		// makePerfectBinaryTree를 호출하기 전에 _size는 무시됨
		cout << "--- Case 1: Perfect Binary Tree Test (2^n-1 nodes) ---\n";
		makePerfectBinaryTree();
		// 실제 삽입할 노드의 개수를 _size에 다시 저장
		_size = v_data.size();
		break;
	}
	case 2:
	default:
	{
		cout << "--- Case 2: Pure Random Test (" << _size << " nodes) ---\n";
		// 3. 랜덤 데이터 생성 및 삽입 순서로 사용
		makeListWithRand(_size, v_data);
		break;
	}
	}
}

void TestTree::makeListWithRand(int s, vector<int>& v)
{
	v.clear();
	// 0부터 10000 범위의 난수를 사용하여 중복 발생 확률을 낮춤
	const int MAX_RAND = 10000;
	while (s > 0)
	{
		int randnum = rand() % MAX_RAND;
		// find 대신 std::set을 사용하여 중복 확인 효율을 높일 수 있지만, 
		// 현재는 주어진 코드를 최소한으로 수정합니다.
		if (find(v.begin(), v.end(), randnum) == v.end())
		{
			v.push_back(randnum);
			s--;
		}
	}
}
