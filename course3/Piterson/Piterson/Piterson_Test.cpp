////피터슨 락
//#include <iostream>
//#include <thread>
//#include <vector>
//#include <Windows.h> 
//#include <intrin.h>
//using namespace std;
//
////void flush_cache_msvc(const void* addr) {
////    // CLFLUSH 명령어를 실행하는 내장 함수 호출
////    _mm_clflush(addr);
////}
//
//// ------------------------------------------------------------------
//// 스레드 개수 및 반복 횟수
//// ------------------------------------------------------------------
//int NUM_THREADS = 2;
//int ITERATIONS_PER_THREAD = 200000;
//
//// ------------------------------------------------------------------
//// 공유 자원 및 락 변수
//// ------------------------------------------------------------------
//int g_local = 0;
//volatile long Flag[2] = { 0, 0 };
//volatile long Turn = 0;
//
//
////-------------------------------------------------------
////동시 진입 검증을 위한 변수
////같이 락 걸리지 않게 다른 캐시라인으로 분리
////-------------------------------------------------------
//struct E {
//    long enter = 0;
//    char padding[60] = { 0, };
//};
//alignas(64) E e;
//
//
////-------------------------------------------------------
////아무데나 인터락 걸어보는 테스트 용 변수
////-------------------------------------------------------
//struct Test {
//    long test = 0;
//    char padding[60];
//};
//alignas(64) Test t;
//
//
////-------------------------------------------------------
////동시 진입점 표시
////-------------------------------------------------------
//bool findTogetherEnter = false;
//
//
//
//// ------------------------------------------------------------------
//// 피터슨 락 구현 함수
//// ------------------------------------------------------------------
//void AcquireLockA(int trash)
//{
//    //_InterlockedExchange(&Flag[0], 1);
//    Flag[0] = 1;
//    //_InterlockedExchange(&Turn, 0);
//    trash++;
//    Turn = 0;
//    trash++;
//
//    int w = 0;
//    while (1)
//    {
//        w++;
//
//       //InterlockedCompareExchange(&t.test, 0, 0);
//        trash++;
//        
//        //동시에 Flag 변경 시 먼저 온 스레드 실행
//        int t = Turn;
//        //int t = InterlockedCompareExchange(&Turn, 0, 0);
//        if (t != 0) {
//            //int in = _InterlockedIncrement(&e.enter);
//            int temp = _InterlockedExchange(&e.enter, 1);
//            if (temp == 0)
//            {
//                trash++;
//            }
//            else
//            {
//                findTogetherEnter = true;
//            }
//            break;
//        }
//
//        //다른 스레드의 Flag 변경 확인
//        int f = Flag[1];
//        if (f == 0) {
//            //int in = _InterlockedIncrement(&e.enter);
//            int temp = _InterlockedExchange(&e.enter, 1);
//            if (temp == 0)
//            {
//                trash++;
//            }
//            else
//            {
//                findTogetherEnter = true;
//            }
//            break;
//        }
//
//    }
//
//}
//
//
//void AcquireLockB(int stackEnter)
//{
//    //_InterlockedExchange(&Flag[1], 1);
//    Flag[1] = 1;
//    //_InterlockedExchange(&Turn, 1);
//    Turn = 1;
//
//    while (1)
//    {
//
//        //InterlockedCompareExchange(&t.test, 0, 0);
//
//        //동시에 Flag 변경 시 먼저 온 스레드 실행
//        int t = Turn;
//        //int t = InterlockedCompareExchange( & Turn, 0, 0);
//        if (t != 1) {
//            //int in = _InterlockedIncrement(&e.enter);
//            int temp = _InterlockedExchange(&e.enter, 2);
//            if (temp == 0)
//            {
//                stackEnter++;
//            }
//            else
//            {
//                findTogetherEnter = true;
//            }
//            break;
//        }
//
//        //다른 스레드의 Flag 변경 확인
//        int f = Flag[0];
//        if (f == 0) {
//            //int in = _InterlockedIncrement(&e.enter);
//            int temp = _InterlockedExchange(&e.enter, 2);
//            if (temp == 0)
//            {
//                stackEnter++;
//            }
//            else
//            {
//                findTogetherEnter = true;
//            }
//            break;
//        }
//    }
//}
//
//void ReleaseLockA()
//{
//    //InterlockedCompareExchange(&t.test, 0, 0);
//    _InterlockedExchange(&Flag[0], 0);
//
//    //Flag[0] = 0;
//    //InterlockedCompareExchange(&t.test, 0, 0);
//}
//
//
//void ReleaseLockB()
//{
//    //InterlockedCompareExchange(&t.test, 0, 0);
//    _InterlockedExchange((long*)&Flag[1], 0);
//
//    //Flag[1] = 0;
//    //InterlockedCompareExchange(&t.test, 0, 0);
//}
//// ------------------------------------------------------------------
//
//// 각 스레드가 실행할 함수
//void ThreadWorkerA()
//{
//    int stackEnter = 0;
//    for (int i = 0; i < ITERATIONS_PER_THREAD; i++)
//    {
//        AcquireLockA(stackEnter);
//        //실행 처리할 함수
//
//        //---------------------------------------------------------------------------
//        //임계 영역
//
//        g_local++;
//        //임계 영역
//        //---------------------------------------------------------------------------
//        //만약 증가시키기 전의 값과 달라졌다면 동시 진입이 발생한 것
//        int d = _InterlockedExchange((long*)&e.enter, 0);
//        if (d != 1)
//        {
//            findTogetherEnter = true;
//        }
//        else
//        {
//            stackEnter--;
//        }
//
//
//        ReleaseLockA();
//    }
//
//}
//
//// 각 스레드가 실행할 함수
//void ThreadWorkerB()
//{
//
//    int stackEnter = 0;
//    for (int i = 0; i < ITERATIONS_PER_THREAD; i++)
//    {
//        AcquireLockB(stackEnter);
//        //실행 처리할 함수
//       
//
//        //---------------------------------------------------------------------------
//        //임계 영역
//
//        g_local++;
//
//        //만약 증가시키기 전의 값과 달라졌다면 동시 진입이 발생한 것
//        //int d = _InterlockedDecrement((long*)&e.enter);
//        int d = _InterlockedExchange((long*)&e.enter, 0);
//        if (d != 2)
//        {
//            findTogetherEnter = true;
//        }
//        else
//        {
//            stackEnter--;
//        }
//        //임계 영역
//        //---------------------------------------------------------------------------
//
//        ReleaseLockB();
//    }
//
//}
//
//int main()
//{
//
//    for (int i = 0; i < 100; i++) {
//        thread t1(ThreadWorkerA);
//        thread t2(ThreadWorkerB);
//
//        t1.join();
//        t2.join();
//
//        cout << g_local << "\n";
//    }
//
//
//    return 0;
//}
//
