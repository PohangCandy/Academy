#pragma once
#ifndef  __PROCADEMY_MEMORY_POOL__
#define  __PROCADEMY_MEMORY_POOL__
#include "stdafx.h"

#define USERBIT (0x007fffffffffff)

namespace myMemorypool
{

	template <class DATA>
	class CMemoryPool
	{
	public:

		bool _bPlacementNew;
		int _iCapacity;
		int _iUseCount;

		struct st_STACK_NODE
		{
			DATA d;
			st_STACK_NODE* nextNode;
			CMemoryPool<DATA>* owner;
		};

		st_STACK_NODE* _pTopNode;


		CMemoryPool(int iBlockNum, bool bPlacementNew = false, int iMaxCapacity = 0)
		{
			_iCapacity = iBlockNum;
			_bPlacementNew = bPlacementNew;
			_iUseCount = 0;
			_pTopNode = nullptr;
			_iMaxCapacity = iMaxCapacity;

			for (int i = 0; i < iBlockNum; i++)
			{
				st_STACK_NODE* newNode = (st_STACK_NODE*)malloc(sizeof(st_STACK_NODE));
				memset(newNode, 0, sizeof(st_STACK_NODE));

				if (bPlacementNew)
				{
					new(&newNode->d) DATA();
				}

				newNode->nextNode = _pTopNode;
				newNode->owner = this;
				_pTopNode = newNode;
			}
		}

		virtual	~CMemoryPool()
		{
			// _pTopNode과 nextNode에는 ABA 방지용 상위 17비트 카운터가
			// 포함되어 있으므로, 반드시 하위 47비트만 마스킹하여 사용해야 한다.
			st_STACK_NODE* cur = (st_STACK_NODE*)((long long)_pTopNode & USERBIT);
			st_STACK_NODE* tempNode = nullptr;

			while (cur != nullptr)
			{
				tempNode = (st_STACK_NODE*)((long long)cur->nextNode & USERBIT);

				if (_bPlacementNew)
				{
					cur->d.~DATA();
				}

				free(cur);

				cur = tempNode;
				_iCapacity--;
			}

			_pTopNode = nullptr;
		}

		DATA* Alloc(void)
		{
			st_STACK_NODE* ptop;
			st_STACK_NODE* newtop;
			st_STACK_NODE* UserBit;
			do {

				ptop = (st_STACK_NODE*)InterlockedCompareExchange64(
					(long long*)&_pTopNode,
					0, 0
				);

				long long lower_47bit = ((long long)ptop & USERBIT);
				UserBit = (st_STACK_NODE*)lower_47bit;

				if (UserBit != nullptr)
				{
					newtop = UserBit->nextNode;
				}
				else
				{
					// 하드 캡 체크: 무제한 팽창으로 인한 메모리 고갈 방지
					if (_iMaxCapacity > 0 && _iCapacity >= _iMaxCapacity)
					{
						printf("[MemoryPool] Max capacity reached (%d) - Alloc denied\n", _iMaxCapacity);
						return nullptr;
					}

					newtop = (st_STACK_NODE*)malloc(sizeof(st_STACK_NODE));
					if (newtop == nullptr)
					{
						printf("[MemoryPool] Alloc memory allocation failed!");
						return nullptr;
					}

					memset(newtop, 0, sizeof(st_STACK_NODE));

					if (_bPlacementNew)
					{
						new(&newtop->d) DATA();
					}
					newtop->owner = this;

					InterlockedIncrement((long*)&_iCapacity);
					InterlockedIncrement((long*)&_iUseCount);
					return &newtop->d;
				}

			} while (popCAS(_pTopNode, newtop, ptop) != ptop);

			InterlockedIncrement((long*)&_iUseCount);

			return &UserBit->d;
		}

		st_STACK_NODE* popCAS(st_STACK_NODE*& nTop, st_STACK_NODE*& nNewNode, st_STACK_NODE*& ptop)
		{
			if (ptop == (st_STACK_NODE*)InterlockedCompareExchange64((long long*)&nTop, (long long)nNewNode, (long long)ptop))
			{
				return ptop;
			}
			else
			{
				return nullptr;
			}
		}

		bool Free(DATA* pData)
		{
			if (pData == nullptr) return false;

			st_STACK_NODE* temp = (st_STACK_NODE*)((char*)pData - offsetof(st_STACK_NODE, d));

			if (temp->owner != this)
			{
				printf("[MemoryPool] Wrong pool owner.\n");
				return false;
			}

			long long upper_17bit = InterlockedIncrement((long*)&cnt);

			st_STACK_NODE* newTop = temp;

			long long lower_47bit = ((long long)newTop & USERBIT);
			st_STACK_NODE* UserBit = (st_STACK_NODE*)lower_47bit;
			st_STACK_NODE* ptop;

			newTop = (st_STACK_NODE*)((upper_17bit << (64 - 17)) | lower_47bit);

			do {
				ptop = _pTopNode;
				UserBit->nextNode = ptop;

			} while (pushCAS(_pTopNode, newTop, ptop) != ptop);

			InterlockedDecrement((long*)&_iUseCount);

			return true;
		}

		st_STACK_NODE* pushCAS(st_STACK_NODE*& nTop, st_STACK_NODE*& nNewNode, st_STACK_NODE*& ptop)
		{
			if (ptop == (st_STACK_NODE*)InterlockedCompareExchange64((long long*)&nTop, (long long)nNewNode, (long long)ptop))
			{
				return ptop;
			}
			else
			{
				return nullptr;
			}
		}

		int		GetCapacityCount(void) { return _iCapacity; }
		int		GetUseCount(void) { return _iUseCount; }
		int		GetMaxCapacity(void) { return _iMaxCapacity; }
		void	SetMaxCapacity(int iMax) { _iMaxCapacity = iMax; }

	private:
		int cnt = 0;
		int _iMaxCapacity = 0;	// 0 = 무제한, >0 = 하드 캡
	};
}

#endif
