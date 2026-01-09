#pragma once
//---------------------------------------------------------------------------------------------
// 프로젝트명: 
// 각각의 TLS에 메모리를 스택이나 풀 단위로 할당해줄 공용 메모리 풀
// 
// 목적:
// 0. 각 스레드마다 메모리 풀을 둬서 경합을 줄이기위한 방식이 사용되었다.
// A 스레드 메모리 풀에서 할당된 메모리를 B스레드에서 해제하는 경우
// a) 다시 A스레드의 메모리 풀에 반환한다.
// b) 그냥 B스레드의 메모리 풀에 반환한다.
// c) 공용 풀에 반환한다.
// 
// 1. 성능 하락이 일어나는 지점은 new, delete와 같은 동적 할당과 해제가 일어나는 지점.
// 각
// 
// 방법 : 
// 
// 결론 : 
// 
//---------------------------------------------------------------------------------------------
#ifndef  __PROCADEMY_MEMORY_POOL__
#define  __PROCADEMY_MEMORY_POOL__
#include <new.h>
#include <stdlib.h>
#include <stdio.h>

#define USERBIT (0x007fffffffffff)

namespace procademy
{

	template <class DATA>
	class CPublicMemoryPool
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
		bool m_bPlacementNew;

		//메모리 풀 내부 전체 개수
		int m_iCapacity;

		//사용중인 블럭 개수
		int m_iUseCount;


		struct st_BLOCK_NODE
		{
			//구조체나 객체와 같은 경우 크기가 크면 포인터로 나타내는게 정석임.
			//하지만 포인터를 사용하면 메모리는 주는 대신 노드 추가 생성시 포인터에 대해 다시 동적할당 해줘야 함.
			//-> 이렇게 되면 결국 메모리 풀을 쓰는 의미가 없어짐. 성능이 떨어지게 됨.
			//그러니 메모리를 좀 먹더라도 그냥 데이터형을 그대로 선언해줌.
			DATA d;
			st_BLOCK_NODE* nextNode;

			//다른 인스턴스가 현재 메모리 풀에 반환되는 상황 방지
			CPublicMemoryPool<DATA>* owner;
		};

		// 스택 방식으로 반환된 (미사용) 오브젝트 블럭을 관리.
		//스택의 가장 top에 있는 노드
		st_BLOCK_NODE* m_pTopNode;


		CPublicMemoryPool(int iBlockNum, bool bPlacementNew = false)
		{
			m_iCapacity = iBlockNum;
			m_bPlacementNew = bPlacementNew;
			m_iUseCount = 0;
			m_pTopNode = nullptr;

			//노드를 개수만큼 확보
			for (int i = 0; i < iBlockNum; i++)
			{
				//메모리만 확보
				st_BLOCK_NODE* newNode = (st_BLOCK_NODE*)malloc(sizeof(st_BLOCK_NODE));

				//printf("%d\n",i);
				memset(newNode, 0, sizeof(st_BLOCK_NODE));

				//객체인 경우 생성자 호출
				if (bPlacementNew)
				{
					new(&newNode->d) DATA();
				}

				newNode->nextNode = m_pTopNode;
				newNode->owner = this;
				m_pTopNode = newNode;
			}
		}

		//확보했던 모든 메모리를 해제해줘야 함.
		virtual	~CPublicMemoryPool()
		{
			//m_iUseCount > 0 사용 중인데 해제하는게 괜찮을까?

			st_BLOCK_NODE* tempNode = nullptr;

			while (m_pTopNode != nullptr)
			{
				tempNode = m_pTopNode->nextNode;

				if (m_bPlacementNew)
				{
					m_pTopNode->d.~DATA();
				}

				//여기서 해제하는 노드가 d가 되야 하는거 아닌가?
				free(m_pTopNode);

				m_pTopNode = tempNode;
				m_iCapacity--;
			}
		}

		//////////////////////////////////////////////////////////////////////////
		// 블럭 하나를 할당받는다.  
		//
		// Parameters: 없음.
		// Return: (DATA *) 데이타 블럭 포인터.
		//////////////////////////////////////////////////////////////////////////
		DATA* Alloc(void)
		{
			//멀티스레드를 대비하여 pop을 원자적으로 진행
			st_BLOCK_NODE* ptop;
			st_BLOCK_NODE* newtop;
			st_BLOCK_NODE* UserBit;
			do {

				ptop = (st_BLOCK_NODE*)InterlockedCompareExchange64(
					(long long*)&m_pTopNode,
					0, 0
				);

				//1.맴버 참조는 유저영역 주소(하위 47bit)를 통해 한다.
				long long lower_47bit = ((long long)ptop & USERBIT);
				UserBit = (st_BLOCK_NODE*)lower_47bit;

				if (UserBit != nullptr)
				{
					newtop = UserBit->nextNode;
				}
				//모든 노드가 이미 다 나간 경우
				//아예 새로운 노드 할당
				else
				{
					newtop = (st_BLOCK_NODE*)malloc(sizeof(st_BLOCK_NODE));
					if (newtop == nullptr)
					{
						printf("[MemoryPool] Alloc에서 메모리 할당 실패 발생!");
						return nullptr;
					}

					memset(newtop, 0, sizeof(st_BLOCK_NODE));

					//객체인 경우 생성자 호출
					if (m_bPlacementNew)
					{
						new(&newtop->d) DATA();
					}
					newtop->owner = this;

					//메모리 풀에서 pop한 top의 데이터 값을 반환
					return &newtop->d;
				}

			} while (popCAS(m_pTopNode, newtop, ptop) != ptop);

			InterlockedIncrement((long*)&m_iUseCount);

			//메모리 풀에서 pop한 top의 데이터 값을 반환
			return &UserBit->d;
		}

		st_BLOCK_NODE* popCAS(st_BLOCK_NODE*& nTop, st_BLOCK_NODE*& nNewNode, st_BLOCK_NODE*& ptop)
		{
			if (ptop == (st_BLOCK_NODE*)InterlockedCompareExchange64((long long*)&nTop, (long long)nNewNode, (long long)ptop))
			{
				st_BLOCK_NODE* pt = ptop;

				//아래 작업도 모두 유저비트를 이용해야 함.
				long long lower_47bit = ((long long)ptop & USERBIT);
				st_BLOCK_NODE* UserBit = (st_BLOCK_NODE*)lower_47bit;
				if (UserBit != nullptr)
				{
					long long v1 = (long long)nNewNode;
					long long v2 = (long long)UserBit->nextNode;
					if (v1 != v2) {
						// 여기서 v1과 v2의 값을 16진수로 출력해서 비트 하나하나가 일치하는지 확인
						printf("Diff: %016llx vs %016llx\n", v1, v2);
					}

					if (nNewNode != UserBit->nextNode)
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

		//////////////////////////////////////////////////////////////////////////
		// 사용중이던 블럭을 풀에 반환
		// 동적 해제는 여기서 이뤄지지 않고 소멸자에서 실행
		//
		// Parameters: (DATA *) 블럭 포인터.
		// Return: (BOOL) TRUE, FALSE.
		//////////////////////////////////////////////////////////////////////////
		bool Free(DATA* pData)
		{
			if (pData == nullptr) return false;

			//맴버 d의 오프셋으로 메모리 풀 노드의 시작 주소 계산
			st_BLOCK_NODE* temp = (st_BLOCK_NODE*)((char*)pData - offsetof(st_BLOCK_NODE, d));

			if (temp->owner != this)
			{
				printf("풀에서 다른 객체 감지됨.\n");
				return false;
			}

			//락프리 구조를 위해 메모리 풀에 반환하기 전 stamp찍어서 담아두기
			
			//1.stamp로 사용할 cnt 값 가져오기
			long long upper_17bit = InterlockedIncrement((long*)&cnt);

			//printf("push 진행중\n");
			st_BLOCK_NODE* newTop = temp;

			//2.newTop의 하위 비트 저장
			long long lower_47bit = ((long long)newTop & USERBIT);
			st_BLOCK_NODE* UserBit = (st_BLOCK_NODE*)lower_47bit;
			st_BLOCK_NODE* ptop;

			//3.CAS하기 전에 newTop에 들어갈 주소에 cnt를 나타내는 17비트 세팅
			newTop = (st_BLOCK_NODE*)((upper_17bit << (64 - 17)) | lower_47bit);

			do {
				ptop = m_pTopNode;
				//4.맴버 참조는 유저영역 주소(하위 47bit)를 통해 한다.
				UserBit->nextNode = ptop;

			} while (pushCAS(m_pTopNode, newTop, ptop) != ptop);
			//ptop가 nullptr이고, m_pTopNode이 nullptr이 아닌 경우에도 성립할 수 있음.
			//ptop가 nullptr이고, m_pTopNode이 nullptr이 아니면 interlock으로 걸리지 않나?




			//다시 메모리 풀에 채워주고
			InterlockedDecrement((long*)&m_iUseCount);

			return true;
		}

		st_BLOCK_NODE* pushCAS(st_BLOCK_NODE*& nTop, st_BLOCK_NODE*& nNewNode, st_BLOCK_NODE*& ptop)
		{
			if (ptop == (st_BLOCK_NODE*)InterlockedCompareExchange64((long long*)&nTop, (long long)nNewNode, (long long)ptop))
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
		int		GetCapacityCount(void) { return m_iCapacity; }

		//////////////////////////////////////////////////////////////////////////
		// 현재 사용중인 블럭 개수를 얻는다.
		//
		// Parameters: 없음.
		// Return: (int) 사용중인 블럭 개수.
		//////////////////////////////////////////////////////////////////////////
		int		GetUseCount(void) { return m_iUseCount; }

	private:
				int cnt = 0;
	};
}





















#endif