//---------------------------------------------------------------------------------------------
// 프로젝트명: 
// 멀티 스레드를 이용해 락프리 스택에 push, pop 진행
// 
// 목적: 
// 락프리 스택 구조에서 발견되는 문제 파악 및 해결 방법 강구
// 
// 방법 : 
// 1. 여러 스레드에서 push, pop 하도록 만든다.
// 2. 문제가 발생하는 순간을 관측한다.
// 
// 결론 : 
// 
//---------------------------------------------------------------------------------------------
#include "stdafx.h"
#include "CLockFreeStack.h"
#include "CMemoryViewer.h"
#include <time.h>


#define RANDRNAGE (1000)
#define POPPUSHCOUNT (5)

enum PushOrPop
{
	Epush,
	Epop
};

CLockFreeStack g_myStack;



int makeRandNum()
{
	return rand() % RANDRNAGE;
}

unsigned int __stdcall PushPopThread(LPVOID arg)
{
	CMemoryViewer* pMv = new CMemoryViewer;

	srand(time(NULL));

	
	
	while (1)
	{
		printf("진행중\n");
		//bool push = false;
		//int cnt = POPPUSHCOUNT;
		//while (cnt--)
		//{
		//	switch (push)
		//	{
		//	case 0:
		//			for (int i = 0; i < 100; i++)
		//			{
		//				g_myStack.push(makeRandNum(), pMv);
		//			}
		//		break;
		//	case 1:
		//		for (int i = 0; i < 100; i++)
		//		{
		//			g_myStack.pop(pMv);
		//		}
		//		break;
		//	default:
		//			while (1)
		//			{
		//					printf("말도 안되는게 나옴\n");
		//			}
		//		break;
		//	}
		//}
		//push = !push;
		// 
		int pp = rand() % 2;
		
		switch (pp)
		{
		case Epush:
			for (int i = 0; i < 1000; i++)
			{
				g_myStack.push(makeRandNum(), pMv);
			}
			break;
		case Epop:
			for (int i = 0; i < 1000; i++)
			{
				g_myStack.pop(pMv);
			}
			break;
		default:
			while (1)
			{
				printf("말도 안되는게 나옴\n");
			}
			break;
		}


	}

	delete pMv;
}

unsigned int threadID;

int main()
{
	//CPU 개수 확인
	SYSTEM_INFO si;
	GetSystemInfo(&si);

	HANDLE hThreads[8] = {0};


	for (int i = 0; i < (int)si.dwNumberOfProcessors * 2; i++)
	//for (int i = 0; i < 1; i++)
	{
		hThreads[i] = (HANDLE)_beginthreadex(
			nullptr,
			0,
			PushPopThread,
			nullptr,
			0,
			&threadID
		);

		if (hThreads[i] == NULL) return false;
	}

	WaitForMultipleObjects((int)si.dwNumberOfProcessors * 2, hThreads, true, INFINITE);
	//WaitForMultipleObjects(1, hThreads, true, INFINITE);

	for (int i = 0; i < (int)si.dwNumberOfProcessors * 2; i++)
	//for (int i = 0; i < 1; i++)
	{
		if(hThreads[i] != nullptr)
			CloseHandle(hThreads[i]);
	}

	return 0;
}

