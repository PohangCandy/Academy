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

void TestTree::makeListWithRand(int s, vector<int>& v)
{
	v.clear();
	while (s > 0)
	{
		int randnum = rand() % 100;
		if (find(v.begin(), v.end(), randnum) == v.end())
		{
			v.push_back(randnum);
			s--;
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
