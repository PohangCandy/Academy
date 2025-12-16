#include "CLockFreeStack.h"
#include "stdafx.h"
#include "CMemoryViewer.h"

Node::Node(int i) : data(i), nextNode(nullptr)
{

}

//void CLockFreeStack::push(int i)
//{
//	//printf("push 진행중\n");
//	Node* newNode = new Node(i);
//	Node* ptop;
//
//	do{
//		ptop = _pTop;
//		newNode->nextNode = ptop;
//	
//	} while(pushCAS(_pTop, newNode, ptop) != ptop);
//	
//}

void CLockFreeStack::push(int i,CMemoryViewer* pmv)
{
	//printf("push 진행중\n");
	Node* newNode = new Node(i);
	Node* ptop;

	do {
		ptop = _pTop;
		newNode->nextNode = ptop;

	} while (pushCAS(_pTop, newNode, ptop, pmv) != ptop);
}

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

void CLockFreeStack::pop(CMemoryViewer* pmv)
{
	//printf("pop 진행중\n");
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

	} while (popCAS(_pTop, newtop, ptop, pmv) != ptop);
}

//void CLockFreeStack::pop(int& popData)
//{
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
//	} while (popCAS(_pTop, newtop, ptop, popData) == ptop);
//}

int CLockFreeStack::size()
{
	return _size;
}

bool CLockFreeStack::empty()
{
	return _size == 0;
}

//Node* CLockFreeStack::pushCAS(Node*& nTop, Node*& nNewNode, Node*& ntop)
//{	
//	if (ntop ==(Node*)InterlockedCompareExchange((long*)&nTop, (long)nNewNode, (long)ntop))
//	{
//		InterlockedIncrement((long*)&_size);
//		return ntop;
//	}
//	else
//	{
//		return nullptr;
//	}
//}

Node* CLockFreeStack::pushCAS(Node*& nTop, Node*& nNewNode, Node*& ntop, CMemoryViewer* pmv)
{
	pmv->copy((char*)nNewNode, sizeof(Node*), (char*)ntop, sizeof(Node*));
	if (ntop == (Node*)InterlockedCompareExchange((long*)&nTop, (long)nNewNode, (long)ntop))
	{
		pmv->copy((char*)nNewNode, sizeof(Node*), (char*)ntop, sizeof(Node*));
		InterlockedIncrement((long*)&_size);
		return ntop;
	}
	else
	{
		return nullptr;
	}
}

//Node* CLockFreeStack::popCAS(Node*& nTop, Node*& nNewNode, Node*& ntop, int& popData)
//{
//	if (ntop == (Node*)InterlockedCompareExchange((long*)&nTop, (long)nNewNode, (long)ntop))
//	{
//		if (nNewNode != ntop->nextNode)
//		{
//			printf("ABA문제가 발생했다!\n");
//		}
//
//		Node* pt = ntop;
//		if (ntop != nullptr)
//		{
//			popData = pt->data;
//			delete ntop;
//			ntop = nullptr;
//			InterlockedDecrement((long*)&_size);
//		}
//		else
//		{
//			popData = -1;
//		}
//		return pt;
//	}
//	else
//	{
//		return nullptr;
//	}
//}

//Node* CLockFreeStack::popCAS(Node*& nTop, Node*& nNewNode, Node*& ntop)
//{
//	if (ntop == (Node*)InterlockedCompareExchange((long*)&nTop, (long)nNewNode, (long)ntop))
//	{
//		Node* pt = ntop;
//		if (ntop != nullptr)
//		{
//			if (nNewNode != ntop->nextNode)
//			{
//				printf("ABA문제가 발생했다!\n");
//			}
//
//			delete ntop;
//			ntop = nullptr;
//			InterlockedDecrement((long*)&_size);
//		}
//		return pt;
//	}
//	else
//	{
//		return nullptr;
//	}
//}

Node* CLockFreeStack::popCAS(Node*& nTop, Node*& nNewNode, Node*& ntop, CMemoryViewer* pmv)
{
	pmv->copy((char*)nNewNode, sizeof(Node*), (char*)ntop, sizeof(Node*));
	if (ntop == (Node*)InterlockedCompareExchange((long*)&nTop, (long)nNewNode, (long)ntop))
	{
		pmv->copy((char*)nNewNode, sizeof(Node*), (char*)ntop, sizeof(Node*));
		Node* pt = ntop;
		if (ntop != nullptr)
		{
			if (nNewNode != ntop->nextNode)
			{
				printf("ABA문제가 발생했다!\n");
			}

			delete ntop;
			ntop = nullptr;
			InterlockedDecrement((long*)&_size);
		}
		return pt;
	}
	else
	{
		return nullptr;
	}
}

