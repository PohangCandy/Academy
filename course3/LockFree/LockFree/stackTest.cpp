//---------------------------------------------------------------------------------------------
// 프로젝트명: 내가 만든 락프리 스택이 일반 스택으로서 문제가 없는지 테스트 해본다.
// 
// 목적: 정상적인 std::stack과 비교해서 제대로 된 스택의 역할을 하는가
// 
// 방법 : 
// push와 pop 하면서 추출되는 데이터의 순서, 남아있는 데이터의 크기, 이외의 문제가 없는지 확인해본다.
// 이렇게 하니까 너무 단기간에 push와 pop을 반복하는 것으로 보인다.
// 100번 push한 후, 스택에 있는 모든 데이터를 pop해서 비교하는 로직도 만들어준다.
// 
// 결론 : 
// 문제없이 원활하게 돌아가는 것으로 보임.
// 다음 테스트를 진행하겠음.
// 
//---------------------------------------------------------------------------------------------
#include "stdafx.h"
#include "CLockFreeStack.h"
#include <time.h>

#include <stack>
#include<iostream>
using namespace std;


#define RANDRNAGE (1000)
#define CHECKALLDATATPS (1000)
#define PUSHONLYTRYNUM (100)



CLockFreeStack g_myStack;
stack<int> g_stdStack;

enum PushOrPop
{
	Epush,
	Epop
};

int makeRandNum()
{
	return rand() % RANDRNAGE;
}

void popCheck()
{
	printf("pop진행.\n");
	if (g_stdStack.empty())
	{
		if (!g_myStack.empty())
		{
			while (1)
			{
				printf("pop한 결과가 다름1.\n");
			}
		}
	}
	else
	{
		int t1 = g_stdStack.top();
		g_stdStack.pop();
		int t2;
		g_myStack.pop(t2);
		if (t1 != t2)
		{
			while (1)
			{
				printf("pop한 결과가 다름2.\n");
			}
		}

		if (g_stdStack.size() != g_myStack.size())
		{
			while (1)
			{
				printf("pop한 결과가 다름3.\n");
			}
		}
	}
}

void pushCheck()
{
	printf("push진행.\n");
	int r = makeRandNum();
	g_myStack.push(r);
	g_stdStack.push(r);
	if (g_stdStack.size() != g_myStack.size())
	{
		while (1)
		{
			printf("push한 결과가 다름.\n");
		}
	}
}

void checkAllData()
{
	printf("전체 테스트 진행.\n");
	while (!g_stdStack.empty())
	{
		popCheck();
	}
	if (!g_myStack.empty())
	{
		printf("전체 데이터 비교에서 결과가 다름.\n");
	}
}

int main()
{
	srand(time(NULL));
	while (1)
	{
		int checkAllDataTPS = CHECKALLDATATPS;
		while (checkAllDataTPS--)
		{
			int pp = rand() % 2;
			switch (pp)
			{
			case Epush:
				pushCheck();
				break;
			case Epop:
				popCheck();
				break;
			default:
				while (1)
				{
					printf("말도 안되는게 나옴\n");
				}
				break;
			}
		}
		for (int i = 0; i < PUSHONLYTRYNUM; i++)
		{
			pushCheck();
		}
		checkAllData();
	}
	

	return 0;
}

