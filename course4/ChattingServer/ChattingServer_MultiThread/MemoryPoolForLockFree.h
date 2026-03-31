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


		CMemoryPool(int iBlockNum, bool bPlacementNew = false)
		{
			_iCapacity = iBlockNum;
			_bPlacementNew = bPlacementNew;
			_iUseCount = 0;
			_pTopNode = nullptr;

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
			st_STACK_NODE* tempNode = nullptr;

			while (_pTopNode != nullptr)
			{
				tempNode = _pTopNode->nextNode;

				if (_bPlacementNew)
				{
					_pTopNode->d.~DATA();
				}

				free(_pTopNode);

				_pTopNode = tempNode;
				_iCapacity--;
			}
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

	private:
		int cnt = 0;
	};
}

#endif
