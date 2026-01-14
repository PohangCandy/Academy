#pragma once
//---------------------------------------------------------------------------------------------
// 프로젝트명: 
// 각각의 TLS에 메모리를 스택 단위로 할당하고 반환받기위한 TLS 풀
// 
// 목적:
// 각 스레드마다 메모리 풀을 둬서 경합을 줄이기위한 방식이 사용되었다.
// 
// 방법 : 
// 실제로 사용되는 메모리 풀은 하나의 인스턴스이지만, 각 스레드마다 접근할 수 있는 공간을 나눠서 접근한다.
//  #Alloc하는 방법
//    tls 메모리 풀의 노드 단위인 스택의 top포인터를 스레드에게 할당해준다.
//    스레드는 해당 메모리를 스택에서 다 꺼내쓰기 전까지는 공용 풀에 접근하지 않는다.
//    이때, 공용 풀에서 할당하는 경우 공용 풀의 노드를 해제한다.
//  #Free하는 방법
//    tls 메모리 풀에 새로운 스택 노드를 하나 만들고, 스레드의 Free 스택의 Top 포인터를 넘겨준다.
//   스레드에서 특정 개수만큼 Free 스택을 채우기 전까지는 공용 풀에 반환하지 않는다.
// 
// 결론 : 
// 
//---------------------------------------------------------------------------------------------
#ifndef  __PROCADEMY_MEMORY_POOL__
#define  __PROCADEMY_MEMORY_POOL__
#include "stdafx.h"
#include <iostream>
#include <map>
#include <vector>
//#include "c_MyStack.h"

#define USERBIT (0x007fffffffffff)

//CMemoryPool<> 
//CMemoryPoolTLS<int> p1;
//CMemoryPoolTLS<int> p2;


namespace procademy
{

	template <class DATA>
	class CMemoryPoolTLS
	{
	public:

		//////////////////////////////////////////////////////////////////////////
		// 생성자, 파괴자.
		//
		// Parameters:	(int) 초기 블럭 개수.
		//				(bool) Alloc 시 생성자 / Free 시 파괴자 호출 여부
		// Return:
		//////////////////////////////////////////////////////////////////////////

		//(bool) Alloc 시 생성자 / Free 시 파괴자 호출 여부
		bool _bPlacementNew;

		//메모리 풀 내부 전체 개수
		int _iCapacity;

		//사용중인 블럭 개수
		int _iUseCount;

		//tls 풀에 들어갈 노드의 개수
		const int _iStackNodeNum;

		

		//---------------------------------------
		// 스택 노드
		//  :스레드 별로 주어질 스택의 노드
		//---------------------------------------
		struct st_STACK_NODE
		{
			DATA d;
			st_STACK_NODE* nextNode;

			//Free시키려는 노드와 tlsPool의 owner비교를 스레드에서 비교하여 담는다.
			CMemoryPoolTLS<DATA>* owner;
		};

		//---------------------------------------
		// 스택 노드를 저장할 스택 구조체
		//---------------------------------------
		class c_MyStack {
		public:
			c_MyStack()
			{
				Count = 0;
				_topNode = nullptr;
			}

			void push(st_STACK_NODE* sNode) 
			{
				sNode->nextNode = _topNode;
				_topNode = sNode;
				Count++;
			}

			void pop() 
			{
				st_STACK_NODE* next = _topNode->nextNode;
				_topNode = next;
				Count--;
			}

			int size() { return Count; }

			void clear() 
			{ 
				_topNode = nullptr;
				Count = 0;
			}

			bool empty() 
			{
				return Count == 0;
			}

			//Free에서 사용할때 말고는 사실 필요없음.
			int Count;

			//스택의 top 포인터
			st_STACK_NODE* _topNode;
		};

		//---------------------------------------
		// tls 풀 노드
		//  :공용 풀에서 스택의 top포인터를 저장하고 있는 노드
		//---------------------------------------
		struct st_POOL_NODE
		{
			st_STACK_NODE* _poolNodeStackTop;
				
			//pool의 next노드
			st_POOL_NODE* _pnNode;
		};

		CRITICAL_SECTION templock;
		std::vector<st_POOL_NODE*> v_allocPoolNode;

		//메모리 풀의 가장 top에 있는 스택 노드
		st_POOL_NODE* _pTopNode;

		//---------------------------------------------------
		// tls로 선언한 스레드 별 스택
		// 1. StackForAlloc
		//   a) 공용 풀로부터 스택 포인터를 pop 받기위한 스택
		//   b) nullptr이 될때까지 각 스레드에서 노드 단위로 Alloc
		// 2. StackForFree
		//   a) 공용 풀로 Push 반환하기 위한 스택
		//   b) 스레드에 일정 개수까지 Free해서 담아둠
		//---------------------------------------------------
		struct StackForTLS
		{
			c_MyStack StackForAlloc;
			c_MyStack StackForFree;

			//스레드 소멸시 모든 노드를 공용 풀에 반환
			//=> 다음 번에 공용 풀에서 할당받는 스레드의 노드의 개수는 _iStackNodeNum이 아닐 수 있다!
			~StackForTLS()
			{
				// alloc 스택 반환
				if (!StackForAlloc.empty())
					push(StackForAlloc._topNode);

				// free 스택 반환
				if (!StackForFree.empty())
					push(StackForFree._topNode);
			}
		};

		static thread_local std::map<CMemoryPoolTLS*, StackForTLS> tlsMap;

		StackForTLS* getTlsStack()
		{
			return &tlsMap[this];
		}

		//--------------------------------------------------
		// 메모리 풀 노드에 들어갈 스택 포인터와 스택에 들어갈 노드 세팅
		// 메모리 풀의 스택 노드를 iPoolNum만큼 확보
		// 스택 노드의 노드 개수를 iStackNum만큼 확보
		//--------------------------------------------------
		CMemoryPoolTLS(int iPoolNum, int iStackNum, bool bPlacementNew = false)
		{
			InitializeCriticalSection(&templock);

			_iCapacity = iPoolNum;
			_bPlacementNew = bPlacementNew;
			_iUseCount = 0;
			_iStackNodeNum = iStackNum;
			_pTopNode = nullptr;

			//노드를 개수만큼 확보
			for (int i = 0; i < iPoolNum; i++)
			{
				st_POOL_NODE* npNode = new st_POOL_NODE;

				st_STACK_NODE* stNode = nullptr;
				for (int j = 0; j < iStackNum; j++)
				{
					st_STACK_NODE* nsNode = (st_STACK_NODE*)malloc(sizeof(st_STACK_NODE));
					if (nsNode == nullptr)
					{
						printf("[CMemoryPoolTLS] malloc 실패\n");
						DebugBreak();
					}

					memset(nsNode, 0, sizeof(st_STACK_NODE));

					//객체인 경우 생성자 호출
					if (bPlacementNew)
					{
						new(&nsNode->d) DATA();
					}
					nsNode->owner = this;
					nsNode->nextNode = stNode;
					stNode = nsNode;
				}

				//메모리 풀 노드가 가리키는 스택의 top에 생성한 스택의 top 넣기
				//-> 일일이 push할 필요없음.
				npNode->_poolNodeStackTop = stNode;

				npNode->_pnNode = _pTopNode;
				_pTopNode = npNode;
			}
		}

		//-------------------------------------------------
		// 소멸자에서 해제해줘야 하는 메모리는 2가지
		// 1. 스택 노드 안에 담긴 모든 노드
		// 2. 공용 풀의 스택 포인터 노드
		// 3. tls 맵에서 현재 인스턴스의 공간 삭제
		//-------------------------------------------------
		virtual	~CMemoryPoolTLS()
		{
			st_POOL_NODE* tempPoolNode = nullptr;

			while (_pTopNode != nullptr)
			{
				tempPoolNode = _pTopNode->_pnNode;

				//스택에 있는 모든 노드 해제
				st_STACK_NODE* StackTopNode = tempPoolNode->_poolNodeStackTop;
				st_STACK_NODE* tempStackNode = nullptr;
				while (StackTopNode != nullptr)
				{
					tempStackNode = StackTopNode->nextNode;
					if (_bPlacementNew)
					{
						StackTopNode->d.~DATA();
					}

					free(StackTopNode);
					StackTopNode = tempStackNode;
				}

				//메모리 풀의 노드 해제
				delete(_pTopNode);

				//alloc된 메모리풀 노드 해제(임시)
				for (auto& a : v_allocPoolNode)
				{
					delete a;
				}

				_pTopNode = tempPoolNode;
			}
		}

		//----------------------------------------------------------------------------
		// 스레드 스택 top노드의 DATA 포인터 반환
		// 1. 스레드 풀이 텅 빈 경우, 공용 풀에서 스택 노드의 top 반환받기
		// 2. 스레드 풀에서 top노드 반환하기
		//----------------------------------------------------------------------------
		DATA* Alloc(void)
		{
			c_MyStack* allocstack = getTlsStack()->StackForAlloc;

			//1.
			if (allocstack->empty())
			{
				allocstack->_topNode = pop();
			}

			//2.
			st_STACK_NODE* stNode = allocstack->_topNode;
			allocstack->pop();
			return &stNode->d;
		}


		//---------------------------------------------------------------------------------
		// 공용 메모리 풀에서 스택의 top포인터 꺼내오기
		// 스택을 반환할 필요없다! top만 꺼내오자!
		// 
		// 1. 공용 메모리 풀의 top노드에서 스택의 top 포인터 가져오기
		//  a) 공용 메모리풀의 top이 nullptr인 경우 새로운 스택을 만들어서 해당 스택의 top만 반환
		//  b) top이 있다면 공용 풀에서 해당 노드의 top을 빼내오기
		// 2. 스택의 top을 반환한 후 공용 풀의 노드는 삭제
		//---------------------------------------------------------------------------------
		st_STACK_NODE* pop(void)
		{
			//멀티스레드를 대비해 pop을 원자적으로 진행
			st_POOL_NODE* ptop;
			st_POOL_NODE* newtop;
			st_POOL_NODE* UserBit;
			do {

				ptop = _pTopNode;

				//1.맴버 참조는 유저영역 주소(하위 47bit)를 통해 한다.
				long long lower_47bit = ((long long)ptop & USERBIT);
				UserBit = (st_POOL_NODE*)lower_47bit;

				if (UserBit != nullptr)
				{
					newtop = UserBit->_pnNode;
				}
				else
				{
					//메모리 풀에 있는 모든 노드를 할당했다면, 스택의 만들어서 해당 스택의 top을 반환
					st_STACK_NODE* stNode = nullptr;

					for (int j = 0; j < _iStackNodeNum; j++)
					{
						st_STACK_NODE* nsNode = (st_STACK_NODE*)malloc(sizeof(st_STACK_NODE));

						memset(nsNode, 0, sizeof(st_STACK_NODE));

						//객체인 경우 생성자 호출
						if (_bPlacementNew)
						{
							new(&nsNode->d) DATA();
						}
						nsNode->owner = this;

						nsNode->nextNode = stNode;
						stNode = nsNode;
					}

					//메모리 풀에서 pop한 top의 데이터 값을 반환
					return stNode;
				}

			} while (popCAS(_pTopNode, newtop, ptop) != ptop);

			InterlockedIncrement((long*)&_iUseCount);

			st_STACK_NODE* stNode = UserBit->_poolNodeStackTop;

			//메모리 풀 노드 삭제
			//여기서 삭제가 일어나면 ABA 문제가 발생할 수 있다...
			//그렇다고 삭제를 안하면 누수가 날텐데...
			//결국 여기에 락을 거는 방법 밖에 없나?
			//delete(UserBit);
			EnterCriticalSection(&templock);
			v_allocPoolNode.push_back(UserBit);
			LeaveCriticalSection(&templock);

			//메모리 풀에서 pop한 top의 데이터 값을 반환
			return stNode;
		}

		st_POOL_NODE* popCAS(st_POOL_NODE*& nTop, st_POOL_NODE*& nNewNode, st_POOL_NODE*& ptop)
		{
			if (ptop == (st_POOL_NODE*)InterlockedCompareExchange64((long long*)&nTop, (long long)nNewNode, (long long)ptop))
			{
				st_POOL_NODE* pt = ptop;

				//아래 작업도 모두 유저비트를 이용해야 함.
				long long lower_47bit = ((long long)ptop & USERBIT);
				st_POOL_NODE* UserBit = (st_POOL_NODE*)lower_47bit;
				if (UserBit != nullptr)
				{
					long long v1 = (long long)nNewNode;
					long long v2 = (long long)UserBit->_pnNode;
					if (v1 != v2) {
						// 여기서 v1과 v2의 값을 16진수로 출력해서 비트 하나하나가 일치하는지 확인
						printf("Diff: %016llx vs %016llx\n", v1, v2);
					}

					if (nNewNode != UserBit->_pnNode)
					{
						printf("ABA문제가 발생했다!\n");
					}
				}
				return pt;
			}
			else
			{
				return nullptr;
			}
		}

		//---------------------------------------------------
		// 노드를 스택에 반환하자.
		// 1. 스레드 스택에 노드 반환
		// 2. 스레드 스택이 꽉 찬 경우, 공용 풀에 top을 반환하기
		//---------------------------------------------------
		bool Free(DATA* pData)
		{
			c_MyStack* freestack = getTlsStack()->StackForFree;
			//Data를 오프셋을 통해 st_Stack_Node 로 변환
			st_STACK_NODE* temp = (st_STACK_NODE*)((char*)pData - offsetof(st_STACK_NODE, d));
			
			//owner가 같은지 스레드 풀에서 확인
			if (temp->owner != this) return;

			//1.
			freestack->push(temp);

			//2.
			if (freestack->size() == _iStackNodeNum)
			{
				push(freestack->_topNode);
				freestack->clear();
			}
		}

		//-------------------------------------------------------------------------
		// 공용 풀로 새로운 스택 포인터 push
		// 1. 새로운 공용 풀 노드(스택 포인터) 할당
		// 2. 해당 스택의 top노드 주소를 스레드가 반환한 노드로 세팅한다.
		// 3. 락프리 과정을 거쳐 push한다.
		//-------------------------------------------------------------------------
		bool push(st_STACK_NODE* pstNode)
		{
			//락프리 구조를 위해 메모리 풀에 반환하기 전 stamp찍어서 담아두기
			
			//ABA_1.stamp로 사용할 cnt 값 가져오기
			long long upper_17bit = InterlockedIncrement((long*)&cnt);

			//1.
			//새로운 풀 노드의 스택의 탑 노드를 push받은 노드로 설정
			st_POOL_NODE* newTop = new st_POOL_NODE;
			newTop->_poolNodeStackTop = pstNode;

			//ABA_2.newTop의 하위 비트 저장
			long long lower_47bit = ((long long)newTop & USERBIT);
			st_POOL_NODE* UserBit = (st_POOL_NODE*)lower_47bit;
			st_POOL_NODE* ptop;

			//ABA_3.CAS하기 전에 newTop에 들어갈 주소에 cnt를 나타내는 17비트 세팅
			newTop = (st_POOL_NODE*)((upper_17bit << (64 - 17)) | lower_47bit);

			do {
				ptop = _pTopNode;
				//ABA_4.맴버 참조는 유저영역 주소(하위 47bit)를 통해 한다.
				UserBit->_pnNode = ptop;

			} while (pushCAS(_pTopNode, newTop, ptop) != ptop);


			InterlockedDecrement((long*)&_iUseCount);

			return true;
		}

		st_POOL_NODE* pushCAS(st_POOL_NODE*& nTop, st_POOL_NODE*& nNewNode, st_POOL_NODE*& ptop)
		{
			if (ptop == (st_POOL_NODE*)InterlockedCompareExchange64((long long*)&nTop, (long long)nNewNode, (long long)ptop))
			{
				return ptop;
			}
			else
			{
				return nullptr;
			}
		}


		//////////////////////////////////////////////////////////////////////////
		// 현재 확보 된 블럭 개수를 얻는다. (메모리풀 내부의 전체 개수)
		//
		// Parameters: 없음.
		// Return: (int) 메모리 풀 내부 전체 개수
		//////////////////////////////////////////////////////////////////////////
		int		GetCapacityCount(void) { return _iCapacity; }

		//////////////////////////////////////////////////////////////////////////
		// 현재 사용중인 블럭 개수를 얻는다.
		//
		// Parameters: 없음.
		// Return: (int) 사용중인 블럭 개수.
		//////////////////////////////////////////////////////////////////////////
		int		GetUseCount(void) { return _iUseCount; }

	private:
				int cnt = 0;
	};
}





















#endif