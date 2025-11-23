//---------------------------------------------------------------------------------------------
// 프로젝트명: 
// 멀티 스레드 네트워크 통신에 사용할 L7 메시지 버퍼 설계
// 
// 목적:
// 두 스레드에서 링버퍼를 사용하는 상황에 정상적인 동작을 할 수 있도록 만든다.
// 락 사용하지 않고 문제 없이 작동하도록 만들자.
// 
// 인 큐 스레드 * 2 , 디큐 스레드 * 2로도 테스트 진행해보자.
// 
// 
// 방법 : 
// 한 스레드에선 Enqueue, 다른 스레드에선 Dequeue 반복해 정상으로 작동하는지 테스트
// 최종적으로 Enqueue한 데이터와 Dequeue한 데이터가 1대 1로 맵핑되어야 한다.
// -> 데이터가 깨지거나 없어졌으면 안 된다.
// 
// 
// 
// 메시지를 랜덤으로 생성해서 넣고 뺀다.
// 
// 결론 :
//
//---------------------------------------------------------------------------------------------
#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include <chrono>
#include <atomic>
#include "TestUtils.h"
#include "CRingBuffer.h" // 위에 수정한 헤더 파일\

using namespace std;

// 테스트용 상수
const int BUFFER_SIZE = 1000;
const int TEST_COUNT = 1000;

//dequeue 할 길이를 알아야하므로 결국 메시지 길이가 통일되거나 앞에 헤더 작성 필요.
struct Msg {
    int len;
    string s;
};


//--------------------------------------------------
// Enqueue, Dequeue할 데이터를 담을 자료구조
//--------------------------------------------------
string EnqueData[TEST_COUNT];
string dequeData[TEST_COUNT];



CRingBuffer g_buffer(BUFFER_SIZE);
std::atomic<int> g_enq_count(0);
std::atomic<int> g_deq_count(0);
std::atomic<int> g_total_sent_value(0);



//----------------------------------------------------------------------------
// 메시지 랜덤 생성 함수
//----------------------------------------------------------------------------
Msg makeRandData()
{
    Msg m;

    int len = GetRandomNumber(1, BUFFER_SIZE);
    string enqueueData = GetRandomString(GetRandomNumber(1, len));

    m.len = len;
    m.s = enqueueData;
    return m;
}

//----------------------------------------------------------------------------
// Enqueue (Producer) 스레드 함수
//----------------------------------------------------------------------------
void ProducerThread()
{
    printf(">> Producer 스레드 시작: 총 %d개의 Int 데이터 전송 예정\n", TEST_COUNT);

    for (int i = 1; i <= TEST_COUNT; ++i)
    {
        // 1. 전송할 데이터
        int len = GetRandomNumber(1, BUFFER_SIZE);
        string enqueueData = GetRandomString(GetRandomNumber(1, len));

        // 2. 링 버퍼에 Enqueue (Enqueue 함수 내부에서 블록킹 발생 가능)
        int enqueued_size = g_buffer.Enqueue(&enqueueData, len);

        if (enqueued_size == len)
        {
            g_enq_count++;
        }
        else
        {
            // 이 블록킹 Enqueue 구현에서는 이 분기가 거의 발생하지 않음
            printf("[Producer] Enqueue 실패 (작게 들어감): 요청 %d, 실제 %d\n", len, enqueued_size);
        }

        // 아주 짧은 sleep으로 스케줄링 전환 유도
        std::this_thread::sleep_for(std::chrono::microseconds(1));
    }
    printf("<< Producer 스레드 종료: 총 %d개 전송 완료\n", (int)g_enq_count);
}

//----------------------------------------------------------------------------
// Dequeue (Consumer) 스레드 함수
//----------------------------------------------------------------------------
void ConsumerThread()
{
    printf(">> Consumer 스레드 시작: 총 %d개의 Int 데이터 수신 예정\n", TEST_COUNT);

    char received_data;
    std::vector<int> received_values;
    char expected_value = 1;

    while (g_deq_count < TEST_COUNT)
    {
        // 1. 링 버퍼에서 Dequeue (Dequeue 함수 내부에서 블록킹 발생 가능)
        int dequeued_size = g_buffer.Dequeue((char*)&received_data, DATA_SIZE);

        if (dequeued_size == DATA_SIZE)
        {
            g_deq_count++;
            received_values.push_back(received_data);

            // 2. 데이터 순서 및 값 검증
            if (received_data != expected_value)
            {
                printf("\n[CONSUMER ERROR] 데이터 순서 오류! 예상값: %d, 실제값: %d\n", expected_value, received_data);
                // 오류 발생 후 테스트 중단 가능
                // return; 
            }
            expected_value++;
        }
        else if (dequeued_size > 0 && dequeued_size < DATA_SIZE)
        {
            printf("[CONSUMER ERROR] 부분 데이터 수신 오류: 요청 %d, 실제 %d\n", DATA_SIZE, dequeued_size);
            // return;
        }

        // 아주 짧은 sleep으로 스케줄링 전환 유도
        std::this_thread::sleep_for(std::chrono::microseconds(2));
    }
    printf("<< Consumer 스레드 종료: 총 %d개 수신 완료\n", (int)g_deq_count);

    // 최종 합계 검증
    long long total_received_value = 0;
    for (int val : received_values) {
        total_received_value += val;
    }

    // 전송된 값의 합계: 1 + 2 + ... + 10000 = (10000 * 10001) / 2 = 50,005,000
    long long expected_total = (long long)TEST_COUNT * (TEST_COUNT + 1) / 2;

    if (total_received_value == expected_total)
    {
        printf("\n[TEST RESULT]  최종 데이터 합계 검증 성공: %lld\n", total_received_value);
    }
    else
    {
        printf("\n[TEST RESULT]  최종 데이터 합계 검증 실패! 예상: %lld, 실제: %lld\n", expected_total, total_received_value);
    }
}


int main()
{
    printf("멀티스레드 링 버퍼 테스트 시작 (버퍼 크기: %d)\n", BUFFER_SIZE);

    // 1. 스레드 생성
    std::thread producer(ProducerThread);
    std::thread consumer(ConsumerThread);

    // 2. 스레드 종료 대기
    producer.join();
    consumer.join();

    // 3. 최종 결과 출력
    printf("\n--- 최종 결과 ---\n");
    printf("생산된 데이터 개수: %d\n", (int)g_enq_count);
    printf("소비된 데이터 개수: %d\n", (int)g_deq_count);

    if (g_enq_count == TEST_COUNT && g_deq_count == TEST_COUNT)
    {
        printf("테스트 통과: 모든 데이터가 성공적으로 처리되었습니다.\n");
    }
    else
    {
        printf("테스트 실패: 데이터 손실 또는 처리 불일치 발생.\n");
    }

    return 0;
}