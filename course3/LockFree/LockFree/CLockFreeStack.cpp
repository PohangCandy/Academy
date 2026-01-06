#include "CLockFreeStack.h"
#include "stdafx.h"
#include "CMemoryViewer.h"
#include "MemoryPoolForLockFree.h"

#define USERBIT (0x007fffffffffff)

procademy::CMemoryPool<Node> NodePool(10000, true);

Node::Node() : data(-1), nextNode(nullptr)
{

}

Node::Node(int i) : data(i), nextNode(nullptr)
{

}

void CLockFreeStack::push(int i)
{
	//printf("push 진행중\n");
	Node* newTop = new Node(i);
	Node* ptop;

	do{
		ptop = _pTop;
		newTop->nextNode = ptop;
	
	} while(pushCAS(_pTop, newTop, ptop) != ptop);
	
}

//ABA해결한 push
void CLockFreeStack::push(int i,CMemoryViewer* pmv)
{
	//1.17비트의 cnt 값 가져오기
	long long upper_17bit = InterlockedIncrement((long*)&cnt);

	//printf("push 진행중\n");
	Node* newTop = NodePool.Alloc();
	Node* ptop;

	//2.newTop의 하위 비트 저장
	long long lower_47bit = ((long long)newTop & USERBIT);
	Node* UserBit = (Node*)lower_47bit;
	UserBit->data = i;
	UserBit->nextNode = nullptr;

	//3.CAS하기 전에 newTop에 들어갈 주소에 cnt를 나타내는 17비트 세팅
	newTop = (Node*)((upper_17bit << (64 - 17)) | lower_47bit);

	do {
		ptop = _pTop;
		//4.맴버 참조는 유저영역 주소(하위 47bit)를 통해 한다.
		UserBit->nextNode = ptop;

	} while (pushCAS(_pTop, newTop, ptop, pmv) != ptop);
}

//void CLockFreeStack::push(int i, CMemoryViewer* pmv)
//{
//
//	//printf("push 진행중\n");
//	Node* newTop = new Node(i);
//	Node* ptop;
//
//
//	do {
//		ptop = _pTop;
//		//4.맴버 참조는 유저영역 주소(하위 47bit)를 통해 한다.
//		newTop->nextNode = ptop;
//
//	} while (pushCAS(_pTop, newTop, ptop, pmv) != ptop);
//}

//void CLockFreeStack::pop()
//{
//	//printf("pop 진행중\n");
//	Node* ptop;
//	Node* newtop;
//
//	do {
//		ptop = _pTop;
//
//		if (ptop != nullptr)
//		{
//			newtop = ptop->nextNode;
//		}
//		else
//		{
//			newtop = nullptr;
//		}
//
//	} while (popCAS(_pTop, newtop, ptop) != ptop);
//}


//ABA해결한 pop
void CLockFreeStack::pop(CMemoryViewer* pmv)
{
	//printf("pop 진행중\n");
	Node* ptop;
	Node* newtop;

	do {
		ptop = _pTop;
		//1.맴버 참조는 유저영역 주소(하위 47bit)를 통해 한다.
		long long lower_47bit = ((long long)ptop & USERBIT);
		Node* UserBit = (Node*)lower_47bit;

		if (UserBit != nullptr)
		{
		// 디커밋 문제 발생
			newtop = UserBit->nextNode;
		}
		//if (_pTop != nullptr)
		//{
		// Nullptr문제 발생
		//	newtop = _pTop->nextNode;
		//}
		else
		{
			newtop = nullptr;
		}

	} while (popCAS(_pTop, newtop, ptop, pmv) != ptop);
}


//void CLockFreeStack::pop(CMemoryViewer* pmv)
//{
//	//printf("pop 진행중\n");
//	Node* ptop;
//	Node* newtop;
//
//	do {
//		ptop = _pTop;
//
//		if (ptop != nullptr)
//		{
//		// 디커밋 문제 발생
//			newtop = ptop->nextNode;
//		}
//		//if (_pTop != nullptr)
//		//{
//		// Nullptr문제 발생
//		//	newtop = _pTop->nextNode;
//		//}
//		else
//		{
//			newtop = nullptr;
//		}
//
//	} while (popCAS(_pTop, newtop, ptop, pmv) != ptop);
//}

void CLockFreeStack::pop(int& popData)
{
	Node* ptop;
	Node* newtop;

	do {
		ptop = _pTop;

		if (ptop != nullptr)
		{
			newtop = ptop->nextNode;
		}
		else
		{
			newtop = nullptr;
		}

	} while (popCAS(_pTop, newtop, ptop, popData) == ptop);
}

int CLockFreeStack::size()
{
	return _size;
}

bool CLockFreeStack::empty()
{
	return _size == 0;
}

Node* CLockFreeStack::pushCAS(Node*& nTop, Node*& nNewNode, Node*& ptop)
{	
	if (ptop ==(Node*)InterlockedCompareExchange64((long long*)&nTop, (long long)nNewNode, (long long)ptop))
	{
		InterlockedIncrement((long*)&_size);
		return ptop;
	}
	else
	{
		return nullptr;
	}
}

Node* CLockFreeStack::pushCAS(Node*& nTop, Node*& nNewNode, Node*& ntop, CMemoryViewer* pmv)
{
	//pmv->copy((char*)nNewNode, sizeof(Node*), (char*)ptop, sizeof(Node*));
	if (ntop == (Node*)InterlockedCompareExchange64((long long*)&nTop, (long long)nNewNode, (long long)ntop))
	{
		pmv->copy((char*)nNewNode, sizeof(Node*), (char*)ntop, sizeof(Node*),epush);
		InterlockedIncrement((long*)&_size);
		return ntop;
	}
	else
	{
		return nullptr;
	}
}

Node* CLockFreeStack::popCAS(Node*& nTop, Node*& nNewNode, Node*& ptop, int& popData)
{
	if (ptop == (Node*)InterlockedCompareExchange64((long long*)&nTop, (long long)nNewNode, (long long)ptop))
	{
		if (nNewNode != ptop->nextNode)
		{
			printf("ABA문제가 발생했다!\n");
		}

		Node* pt = ptop;
		if (ptop != nullptr)
		{
			popData = pt->data;
			delete ptop;
			ptop = nullptr;
			InterlockedDecrement((long*)&_size);
		}
		else
		{
			popData = -1;
		}
		return pt;
	}
	else
	{
		return nullptr;
	}
}

//Node* CLockFreeStack::popCAS(Node*& nTop, Node*& nNewNode, Node*& ptop)
//{
//	if (ptop == (Node*)InterlockedCompareExchange((long*)&nTop, (long)nNewNode, (long)ptop))
//	{
//		Node* pt = ptop;
//		if (ptop != nullptr)
//		{
//			if (nNewNode != ptop->nextNode)
//			{
//				printf("ABA문제가 발생했다!\n");
//			}
//
//			delete ptop;
//			ptop = nullptr;
//			InterlockedDecrement((long*)&_size);
//		}
//		return pt;
//	}
//	else
//	{
//		return nullptr;
//	}
//}

//ABA 해결버전
Node* CLockFreeStack::popCAS(Node*& nTop, Node*& nNewNode, Node*& ptop, CMemoryViewer* pmv)
{
	//pmv->copy((char*)nNewNode, sizeof(Node*), (char*)ptop, sizeof(Node*));
	if (ptop == (Node*)InterlockedCompareExchange64((long long*)&nTop, (long long)nNewNode, (long long)ptop))
	{


		//유저비트만 받아서 복사하도록 만들어줘야할까?
		pmv->copy((char*)nNewNode, sizeof(Node*), (char*)ptop, sizeof(Node*), epop);
		Node* pt = ptop;

		//아래 작업도 모두 유저비트를 이용해야 함.
		long long lower_47bit = ((long long)ptop & USERBIT);
		Node* UserBit = (Node*)lower_47bit;
		if (UserBit != nullptr)
		{
			if (nNewNode != UserBit->nextNode)
			{
				printf("ABA문제가 발생했다!\n");
			}

			//UserBit = nullptr;
			NodePool.Free(UserBit);
			//delete UserBit;
			
			InterlockedDecrement((long*)&_size);
		}
		return pt;
	}
	else
	{
		return nullptr;
	}
}

//Node* CLockFreeStack::popCAS(Node*& nTop, Node*& nNewNode, Node*& ptop, CMemoryViewer* pmv)
//{
//	//pmv->copy((char*)nNewNode, sizeof(Node*), (char*)ptop, sizeof(Node*));
//	if (ptop == (Node*)InterlockedCompareExchange64((long long*)&nTop, (long long)nNewNode, (long long)ptop))
//	{
//
//		pmv->copy((char*)nNewNode, sizeof(Node*), (char*)ptop, sizeof(Node*), epop);
//		Node* pt = ptop;
//
//		if (ptop != nullptr)
//		{
//			if (nNewNode != ptop->nextNode)
//			{
//				printf("ABA문제가 발생했다!\n");
//			}
//
//			delete ptop;
//			ptop = nullptr;
//			InterlockedDecrement((long*)&_size);
//		}
//		return pt;
//	}
//	else
//	{
//		return nullptr;
//	}
//}
