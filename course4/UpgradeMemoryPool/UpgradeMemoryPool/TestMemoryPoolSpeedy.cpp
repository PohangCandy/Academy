#include "CMemoryPoolTLS.h"
#include "CLockFreeStack.h"
#include "CMemoryViewer.h"
#include <stack>
#include <thread>
#include <chrono>

CMemoryViewer cmv;

CLockFreeStack AllocByAThread;
CLockFreeStack AllocByBThread;

procademy::CMemoryPoolTLS<int> cmem(100, 100, false);

// 첫 번째 스레드에서 실행할 함수
void ThreadFuncA()
{
    while (1)
    {
        int r = rand() % 2;
        switch (r)
        {
        case 0:
        {
            int* newi = cmem.Alloc();
            AllocByAThread.push(*newi);
            break;
        }

        case 1:
        {
            int* ptop = AllocByBThread.pop(&cmv);
            if (ptop != nullptr)
            {
                cmem.Free(ptop);
            }
            break;
        }

        }
    }
}

// 두 번째 스레드에서 실행할 함수
void ThreadFuncB()
{
    while (1)
    {
        int r = rand() % 2;
        switch (r)
        {
        case 0:
        {
            int* newi = cmem.Alloc();
            AllocByBThread.push(*newi);
            break;
        }

        case 1:
        {
            int* ptop = AllocByAThread.pop(&cmv);
            if (ptop != nullptr)
            {
                cmem.Free(ptop);
            }
            break;

        }
        }
    }

}

int main()
{
    // 스레드 생성 (각각 다른 함수 지정)
    std::thread t1(ThreadFuncA);
    std::thread t2(ThreadFuncB);

    // 스레드가 끝날 때까지 대기
    t1.join();
    t2.join();

    std::cout << "All threads finished." << std::endl;
    return 0;
}