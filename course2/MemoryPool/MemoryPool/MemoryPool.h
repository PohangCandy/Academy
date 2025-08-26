/*---------------------------------------------------------------

	procademy MemoryPool.

	메모리 풀 클래스 (오브젝트 풀 / 프리리스트)
	특정 데이타(구조체,클래스,변수)를 일정량 할당 후 나눠쓴다.

	- 사용법.

	procademy::CMemoryPool<DATA> MemPool(300, FALSE);
	DATA *pData = MemPool.Alloc();

	pData 사용

	MemPool.Free(pData);

				
----------------------------------------------------------------*/
#ifndef  __PROCADEMY_MEMORY_POOL__
#define  __PROCADEMY_MEMORY_POOL__
#include <new.h>
#include <stdlib.h>
#include <stdio.h>

namespace procademy
{

	template <class DATA>
	class CMemoryPool
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
			CMemoryPool<DATA>* owner;
		};

		// 스택 방식으로 반환된 (미사용) 오브젝트 블럭을 관리.
		//스택의 가장 top에 있는 노드
		st_BLOCK_NODE* m_pFreeNode;


		CMemoryPool(int iBlockNum, bool bPlacementNew = false)
		{
			m_iCapacity = iBlockNum;
			m_bPlacementNew = bPlacementNew;
			m_iUseCount = 0;
			m_pFreeNode = nullptr;

			//노드를 개수만큼 확보
			for (int i = 0; i < iBlockNum; i++)
			{
				//지금은 메모리만 확보
				st_BLOCK_NODE* newNode = (st_BLOCK_NODE*)malloc(sizeof(st_BLOCK_NODE));
				newNode->nextNode = m_pFreeNode;
				m_pFreeNode = newNode;
			}
		}

		//확보했던 모든 메모리를 해제해줘야 함.
		virtual	~CMemoryPool()
		{
			//m_iUseCount > 0 사용 중인데 해제하는게 괜찮을까?

			st_BLOCK_NODE* tempNode = nullptr;

			while (m_pFreeNode != nullptr)
			{
				tempNode = m_pFreeNode->nextNode;
				free(m_pFreeNode);
				m_pFreeNode = tempNode;
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
			//일단 지금 사용가능 용량을 확인해봐야 할거 같은데?
			//전체 용량 - 사용 중인 용량 == 0일 경우 GetCapacityCount() - GetUseCount() == 0
			//새로운 노드를 할당받아야 함.
			if (m_pFreeNode == nullptr)
			{
				st_BLOCK_NODE* newNode = (st_BLOCK_NODE*)malloc(sizeof(st_BLOCK_NODE));
				newNode->nextNode = m_pFreeNode;
				m_pFreeNode = newNode;
				m_iCapacity++;
			}

			//클래스 소유주 확인을 위한 작업
			m_pFreeNode->owner = this;

			//스택 가장 위쪽 메모리를 할당해줌.
			DATA* data = &m_pFreeNode->d;
			//그리고 객체일 경우 생성자 호출
			if (m_bPlacementNew)
			{
				new(data) DATA();
			}

			//풀은 다시 다음 메모리 가리킴.
			st_BLOCK_NODE* temp = m_pFreeNode;
			m_pFreeNode = temp->nextNode;

			m_iUseCount++;

			return data;
		}

		//////////////////////////////////////////////////////////////////////////
		// 사용중이던 블럭을 해제한다.
		//
		// Parameters: (DATA *) 블럭 포인터.
		// Return: (BOOL) TRUE, FALSE.
		//////////////////////////////////////////////////////////////////////////
		bool Free(DATA* pData)
		{
			st_BLOCK_NODE* temp = (st_BLOCK_NODE*)pData;
			if (temp->owner != this)
			{
				printf("풀에서 다른 객체 감지됨.\n");
				return false;
			}
			//객체라면 소멸자 호출
			if (m_bPlacementNew)
			{
				pData->~DATA();
			}
			//다시 메모리 풀에 채워주고
			m_iUseCount--;
			//데이터를 가리키는 포인터 지점이 다시 넣을 노드
			//현재 탑을 가리키는 노드의 탑에 넣음.
			
			temp->nextNode = m_pFreeNode;
			m_pFreeNode = temp;

			return true;
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
	};

}





















#endif