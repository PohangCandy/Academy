//피터슨 락
#include <iostream>
#include <thread>
#include <vector>
#include <Windows.h> 
#include <intrin.h>
using namespace std;

// ------------------------------------------------------------------
// 스레드 개수 및 반복 횟수
// ------------------------------------------------------------------
int NUM_THREADS = 2;
int ITERATIONS_PER_THREAD = 200000;

// ------------------------------------------------------------------
// 공유 자원 및 락 변수
// ------------------------------------------------------------------
int g_local = 0;
volatile long Flag[2] = { 0, 0 };
volatile long Turn = 0;


//-------------------------------------------------------
//동시 진입 검증을 위한 변수
//같이 락 걸리지 않게 다른 캐시라인으로 분리
//-------------------------------------------------------
struct E {
    long enter = 0;
    char padding[60] = { 0, };
};
alignas(64) E e;


//-------------------------------------------------------
//아무데나 인터락 걸어보는 테스트 용 변수
//-------------------------------------------------------
struct Test {
    long test = 0;
    char padding[60];
};
alignas(64) Test tt;


//-------------------------------------------------------
//동시 진입점 표시
//-------------------------------------------------------
bool findTogetherEnter = false;



// ------------------------------------------------------------------
// 피터슨 락 구현 함수
// ------------------------------------------------------------------
void ThreadWorkerA()
{
    int trash = 0;
    for (int i = 0; i < ITERATIONS_PER_THREAD; i++)
    {
        int t = -100;
        int f1 = -100;
        int f0 = -100;
        //----------------------------------------------------------------
        //Lock 
        // ---------------------------------------------------------------
         
        //_InterlockedExchange(&Flag[0], 1);
        Flag[0] = 1;

        //_InterlockedExchange(&Turn, 0);
        Turn = 0;


        //_InterlockedExchange(&tt.test, 0);
        atomic_thread_fence(memory_order_seq_cst);
        //_mm_sfence();
        //_mm_lfence();
        //_mm_mfence();
        //MemoryBarrier();

        while (1)
        {
            t = Turn;
            //int t = InterlockedCompareExchange(&Turn, 0, 0);
            if (t != 0) {
                int TreadBAlreadyEnter = _InterlockedExchange(&e.enter, 1);
                if (TreadBAlreadyEnter == 0)
                {
                    trash++;
                }
                else
                {
                    findTogetherEnter = true;
                }
                break;
            }

            f0 = Flag[0];
            f1 = Flag[1];
            if (f1 == 0) {
                int TreadBAlreadyEnter = _InterlockedExchange(&e.enter, 1);
                if (TreadBAlreadyEnter == 0)
                {
                    trash++;
                }
                else
                {
                    findTogetherEnter = true;
                }
                break;
            }
        }
        
        //---------------------------------------------------------------------------
        //임계 영역

        g_local++;

        //int OnlyAEnter = _InterlockedExchange((long*)&e.enter, 0);
        //if (OnlyAEnter != 1)
        if (_InterlockedExchange((long*)&e.enter, 0) != 1)
        {
            findTogetherEnter = true;
        }
        else
        {
            trash--;
        }
        //임계 영역
        //---------------------------------------------------------------------------

        //---------------------------------------------------------------------------
        // UnLock
        //---------------------------------------------------------------------------
        //InterlockedCompareExchange(&t.test, 0, 0);
        //_InterlockedExchange(&Flag[0], 0);

        Flag[0] = 0;
        //InterlockedCompareExchange(&t.test, 0, 0);
    }

}

// 각 스레드가 실행할 함수
void ThreadWorkerB()
{

    int trash = 0;
    for (int i = 0; i < ITERATIONS_PER_THREAD; i++)
    {
        int t = -100;
        int f0 = -100;
        int f1 = -100;
        //----------------------------------------------------------------------------
        // Lock
        //----------------------------------------------------------------------------
         //_InterlockedExchange(&Flag[1], 1);
        Flag[1] = 1;
        //_InterlockedExchange(&Turn, 1);
        Turn = 1;

        atomic_thread_fence(memory_order_seq_cst);
       //_InterlockedExchange(&tt.test, 0);
       //_mm_sfence();
       //_mm_lfence();
        //_mm_mfence();
        //MemoryBarrier();

        while (1)
        {
            t = Turn;
            //int t = InterlockedCompareExchange( & Turn, 0, 0);
            if (t != 1) {
                int TreadAAlreadyEnter = _InterlockedExchange(&e.enter, 2);
                if (TreadAAlreadyEnter == 0)
                {
                    trash++;
                }
                else
                {
                    findTogetherEnter = true;
                }
                break;
            }

            f0 = Flag[0];
            f1 = Flag[1];
            if (f0 == 0) {
                int TreadAAlreadyEnter = _InterlockedExchange(&e.enter, 2);
                if (TreadAAlreadyEnter == 0)
                {
                    trash++;
                }
                else
                {
                    findTogetherEnter = true;
                }
                break;
            }

        }

        //---------------------------------------------------------------------------
        //임계 영역

        g_local++;

        //int OnlyBEnter = _InterlockedExchange(&e.enter, 0);
        //if (OnlyBEnter != 2)
        if (_InterlockedExchange((long*)&e.enter, 0) != 2)
        {
            findTogetherEnter = true;
        }
        else
        {
            trash--;
        }
        //임계 영역
        //---------------------------------------------------------------------------

        //----------------------------------------------------------------------------
        // UnLock
        //----------------------------------------------------------------------------
        //InterlockedCompareExchange(&t.test, 0, 0);
        //_InterlockedExchange((long*)&Flag[1], 0);

        Flag[1] = 0;
        //InterlockedCompareExchange(&t.test, 0, 0);
    }

}

int main()
{

    for (int i = 0; i < 100; i++) {
        g_local = 0;
        thread t1(ThreadWorkerA);
        thread t2(ThreadWorkerB);

        t1.join();
        t2.join();

        cout << g_local << "\n";
    }


    return 0;
}

