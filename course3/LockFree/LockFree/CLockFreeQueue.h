#pragma once
#include "stdafx.h"
#include "MemoryPoolForLockFree.h"

#define USERBIT (0x007fffffffffff)

template <class T>
class QueueT
{
private:
    long _size;

    struct Node
    {
        T data;
        Node* next;
    };

    Node* _head;        // 시작노드를 포인트한다.
    Node* _tail;        // 마지막노드를 포인트한다.

    procademy::CMemoryPool<Node> NodePool{ 10000, true };

public:
    QueueT()
    {
        _size = 0;
        _head = NodePool.Alloc();
        _head->next = NULL;
        _tail = _head;
    }

    void Enqueue(T t)
    {
        Node* node = NodePool.Alloc();
        //풀에서 꺼낸 노드의 유저비트 만큼을 활용
        long long lower_47bit = ((long long)node & USERBIT);
        Node* UserBit = (Node*)lower_47bit;

        UserBit->data = t;
        UserBit->next = NULL;

        while (true)
        {
            Node* tail = _tail;
            Node* next = tail->next;

            if (next == NULL)
            {
                if (InterlockedCompareExchangePointer((PVOID*)&tail->next, node, nullptr) == next)
                {
                    InterlockedCompareExchangePointer((PVOID*)&_tail, node, tail); //<< 실패의 경우 그 이유 추적
                        break;
                }
            }
        }

        InterlockedExchangeAdd(&_size, 1);
    }

    int Dequeue(T& t)
    {
        if (_size == 0)
            return -1;

        while (true)
        {
            Node* head = _head;
            //풀에서 꺼낸 노드의 유저비트 만큼을 활용
            long long lower_47bit = ((long long)head & USERBIT);
            Node* UserBit = (Node*)lower_47bit;
            Node* next = UserBit->next;

            if (next == NULL)
            {
                return -1;
            }
            else
            {
                if (InterlockedCompareExchangePointer((PVOID*)&_head, next, head) == head)
                {
                    lower_47bit = ((long long)next & USERBIT);
                    UserBit = (Node*)lower_47bit;
                    t = UserBit->data;
                    NodePool.Free(head);
                    break;
                }
            }
        }
        InterlockedExchangeAdd(&_size, -1);
        return 0;
    }
};

