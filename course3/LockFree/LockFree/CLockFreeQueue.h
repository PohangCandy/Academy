#pragma once
#include "stdafx.h"

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

public:
    QueueT()
    {
        _size = 0;
        _head = new Node;
        _head->next = NULL;
        _tail = _head;
    }

    void Enqueue(T t)
    {
        Node* node = new Node;
        node->data = t;
        node->next = NULL;

        while (true)
        {
            Node* tail = _tail;
            Node* next = tail->next;

            if (next == NULL)
            {
                if (InterlockedCompareExchangePointer((PVOID*)&tail->next, node, null) == next)
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
            Node* next = head->next;

            if (next == NULL)
            {
                return -1;
            }
            else
            {
                if (InterlockedCompareExchangePointer((PVOID*)&_head, next, head) == head)
                {
                    t = next->data;
                    delete head;
                    break;
                }
            }
        }
        InterlockedExchangeAdd(&_size, -1);
        return 0;
    }
};