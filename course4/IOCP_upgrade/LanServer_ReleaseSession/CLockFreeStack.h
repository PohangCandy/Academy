//---------------------------------------------------------------------------------------------
// 프로젝트명: 락없이 동기화가 가능한 락프리 스택
// 
// 목적: 락 없이 동기화 가능한 구조의 스택을 만들어서, 네트워크 송수신 큐를 대체한다.
// 이를 통해 성능 향상을 꾀할 수 있다.
// 
// 방법 : 
// Push -> 새로 만든 노드의 next를 top으로 가리키게 하고, 현재의 Top과 top가 일치하면 push한다.
// Pop -> 제거할 top의 next를 newtop에 담아두고, 현재의 Top이 top와 일치하면 pop한다.
// 
// 결론 : 1111
// 
// 
//---------------------------------------------------------------------------------------------
#pragma once

class CMemoryViewer;

class Node
{
public:
	Node();
	Node(int i);
	~Node() {};
	Node* nextNode;
	int data;	
};

class CLockFreeStack {
public:
	CLockFreeStack();
	~CLockFreeStack();
	void push(int i);
	int* pop();
	int size();
	bool empty();
	Node* pushCAS(Node*& dest, Node*& exchange, Node*& compare, CMemoryViewer* pmv);

	Node* popCAS(Node*& dest, Node*& exchange, Node*& compare, CMemoryViewer* pmv);

private:
	int cnt;
	int _size = 0;
	Node* _pTop = nullptr;
	CMemoryViewer* _localMv;

	//void push(int i);
	//void pop();
	//void pop(int& popData);
	//Node* pushCAS(Node*& dest, Node*& exchange, Node*& compare);
	//Node* popCAS(Node*& dest, Node*& exchange, Node*& compare, int& popData);
	//Node* popCAS(Node*& dest, Node*& exchange, Node*& compare);
};