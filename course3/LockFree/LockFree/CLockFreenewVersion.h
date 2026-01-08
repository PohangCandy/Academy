#pragma once
#pragma once
#include <windows.h>
#include "MemoryPoolnewVersion.h" // 앞서 만든 태깅 메모리풀

#define USERBIT (0x00007FFFFFFFFFFF)

template <class T>
class QueueT
{
private:
    struct Node
    {
        T data;
        Node* next; // 메모리풀에서 꺼낸 '태깅된 포인터'가 저장됨
    };

    // 큐의 헤드와 타이틀도 태깅된 포인터 상태를 유지합니다.
    Node* volatile _head;
    Node* volatile _tail;
    volatile long _size;

    procademy::CMemoryPool<Node> NodePool;

    // 태깅된 포인터에서 실제 접근 가능한 주소를 추출하는 헬퍼
    Node* GetPurePtr(Node* tagged) {
        return (Node*)((long long)tagged & USERBIT);
    }

public:
    QueueT() : NodePool(10000, true), _size(0)
    {
        // 더미 노드 할당
        Node* dummy = NodePool.Alloc();
        // Alloc이 반환하는 값은 이미 태깅되어 있을 수 있음
        GetPurePtr(dummy)->next = nullptr;
        _head = _tail = dummy;
    }

    void Enqueue(T t) {
        Node* newNodeTagged = NodePool.Alloc();
        GetPurePtr(newNodeTagged)->data = t;
        GetPurePtr(newNodeTagged)->next = nullptr;

        while (true) {
            Node* tailTagged = _tail;
            Node* tailPure = GetPurePtr(tailTagged);
            Node* nextTagged = tailPure->next;

            if (tailTagged != _tail) continue;

            if (nextTagged == nullptr) {
                // [핵심] tail->next에 '태깅된' 새 노드를 넣어야 함
                if (InterlockedCompareExchangePointer((PVOID*)&tailPure->next, newNodeTagged, nullptr) == nullptr) {
                    // 성공 시 _tail을 newNodeTagged로 교체
                    InterlockedCompareExchangePointer((PVOID*)&_tail, newNodeTagged, tailTagged);
                    break;
                }
            }
            else {
                // 다른 스레드가 next는 연결했는데 tail은 안 밀었을 때 도와줌
                InterlockedCompareExchangePointer((PVOID*)&_tail, nextTagged, tailTagged);
            }
        }
        InterlockedIncrement(&_size);
    }

    int Dequeue(T& t)
    {
        while (true)
        {
            Node* headTagged = _head;
            Node* tailTagged = _tail;
            Node* headPure = GetPurePtr(headTagged);
            Node* nextTagged = headPure->next; // Dummy의 다음 노드

            // 1. 머리 읽기 일관성 체크
            if (headTagged != _head) continue;

            if (headPure == GetPurePtr(tailTagged))
            {
                // 큐가 비어있는가?
                if (nextTagged == nullptr) return -1;

                // [Helper 로직] tail이 뒤처졌다면 이동 시도
                // 여기서 실패하더라도 continue를 통해 최신 head/tail을 다시 읽어야 함
                InterlockedCompareExchangePointer((PVOID*)&_tail, nextTagged, tailTagged);
                continue; // 다시 처음부터 읽기
            }
            else
            {
                // 데이터 노드가 유효한지 확인
                if (nextTagged == nullptr) continue;

                t = GetPurePtr(nextTagged)->data;

                // 2. Head 이동 (성공 시 Pop 완료)
                if (InterlockedCompareExchangePointer((PVOID*)&_head, nextTagged, headTagged) == headTagged)
                {
                    NodePool.Free(headPure);
                    InterlockedDecrement(&_size);
                    return 0;
                }
            }
        }
    }
};