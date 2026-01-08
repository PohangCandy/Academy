#pragma once
#pragma once
#ifndef __PROCADEMY_MEMORY_POOL__
#define __PROCADEMY_MEMORY_POOL__

#include <new.h>
#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <stddef.h>

// Windows x64 유저 모드 주소 마스크 (47비트)
#define USERBIT (0x00007FFFFFFFFFFF)

namespace procademy
{
    template <class DATA>
    class CMemoryPool
    {
    public:
        struct st_BLOCK_NODE
        {
            DATA d;
            st_BLOCK_NODE* nextNode; // 이 nextNode는 순수 주소(Masking된)로 관리하는 것이 편리합니다.
            CMemoryPool<DATA>* owner;
        };

    private:
        // [17bit Tag | 47bit Address]가 합쳐진 상태로 관리됩니다.
        volatile long long m_pTopNode;

        int m_iCapacity;
        int m_iUseCount;
        bool m_bPlacementNew;
        volatile long m_cnt; // 태그 생성용 카운터

        // 태깅 유틸리티
        st_BLOCK_NODE* GetPurePtr(long long tagged) { return (st_BLOCK_NODE*)(tagged & USERBIT); }
        long long MakeTagged(st_BLOCK_NODE* ptr, long long tag) {
            return ((long long)ptr & USERBIT) | (tag << 47);
        }
        long long GetTag(long long tagged) { return (unsigned long long)tagged >> 47; }

    public:
        CMemoryPool(int iBlockNum, bool bPlacementNew = false)
            : m_iCapacity(iBlockNum), m_bPlacementNew(bPlacementNew), m_iUseCount(0), m_cnt(0), m_pTopNode(0)
        {
            for (int i = 0; i < iBlockNum; i++)
            {
                st_BLOCK_NODE* newNode = (st_BLOCK_NODE*)malloc(sizeof(st_BLOCK_NODE));
                memset(newNode, 0, sizeof(st_BLOCK_NODE));

                if (m_bPlacementNew) new(&newNode->d) DATA();

                newNode->owner = this;

                // 초기화 시에는 태그 0으로 쌓음
                long long currentTop = m_pTopNode;
                newNode->nextNode = GetPurePtr(currentTop);
                m_pTopNode = MakeTagged(newNode, 0);
            }
        }

        DATA* Alloc(void)
        {
            long long currentTop;
            long long nextTop;
            st_BLOCK_NODE* pNode;

            do {
                currentTop = m_pTopNode;
                pNode = GetPurePtr(currentTop);

                if (pNode == nullptr) {
                    // 풀이 비었을 때 새 메모리 할당 (태그 없이 순수 할당 후 반환)
                    st_BLOCK_NODE* newNode = (st_BLOCK_NODE*)malloc(sizeof(st_BLOCK_NODE));
                    if (newNode == nullptr) return nullptr;
                    memset(newNode, 0, sizeof(st_BLOCK_NODE));
                    if (m_bPlacementNew) new(&newNode->d) DATA();
                    newNode->owner = this;
                    newNode->nextNode = nullptr;

                    InterlockedIncrement((long*)&m_iUseCount);
                    return &newNode->d;
                }

                // [중요] ABA 방어: 다음 탑을 설정할 때 현재 태그 + 1을 부여함
                // pNode->nextNode는 생성/Free 시점에 순수 주소로 저장되어 있어야 함
                nextTop = MakeTagged(pNode->nextNode, GetTag(currentTop) + 1);

            } while (InterlockedCompareExchange64(&m_pTopNode, nextTop, currentTop) != currentTop);

            InterlockedIncrement((long*)&m_iUseCount);
            return &pNode->d;
        }

        bool Free(DATA* pData)
        {
            if (pData == nullptr) return false;

            st_BLOCK_NODE* temp = (st_BLOCK_NODE*)((char*)pData - offsetof(st_BLOCK_NODE, d));
            if (temp->owner != this) return false;

            long long currentTop;
            long long nextTop;

            // 태그를 위한 카운터 증가
            long long newTag = (unsigned long long)InterlockedIncrement(&m_cnt);

            do {
                currentTop = m_pTopNode;
                // 현재 Top의 순수 주소를 nextNode에 저장
                temp->nextNode = GetPurePtr(currentTop);

                // 새로운 Node 주소에 새로운 태그를 입힘
                nextTop = MakeTagged(temp, newTag);

            } while (InterlockedCompareExchange64(&m_pTopNode, nextTop, currentTop) != currentTop);

            InterlockedDecrement((long*)&m_iUseCount);
            return true;
        }

        virtual ~CMemoryPool()
        {
            st_BLOCK_NODE* curr = GetPurePtr(m_pTopNode);
            while (curr != nullptr)
            {
                st_BLOCK_NODE* next = curr->nextNode;
                if (m_bPlacementNew) curr->d.~DATA();
                free(curr);
                curr = next;
            }
        }
    };
}
#endif