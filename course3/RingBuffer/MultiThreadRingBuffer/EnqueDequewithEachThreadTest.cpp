//---------------------------------------------------------------------------------------------
// 프로젝트명: 
// 멀티 스레드 네트워크 통신에 사용할 L7 메시지 버퍼 설계
// 
// 목적:
// 멀티 스레드에서 링버퍼를 공유하는 상황에 정상적인 동작을 할 수 있도록 만든다.
// 
// 방법 : 
// 한 스레드에선 Enqueue, 다른 스레드에선 Dequeue 반복해 정상으로 작동하는지 테스트
// 1. 최종적으로 Enqueue한 횟수와 Dequeue한 횟수가 같아야 함.
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
#include "CRingBuffer.h" // 위에 수정한 헤더 파일

// 테스트용 상수
const int BUFFER_SIZE = 1000;
const int TEST_COUNT = 10000;
const int DATA_SIZE = 1; // char 1바이트 크기

CRingBuffer g_buffer(BUFFER_SIZE);
std::atomic<int> g_enq_count(0);
std::atomic<int> g_deq_count(0);
std::atomic<int> g_total_sent_value(0);

//메시지 구조체
struct Msg {
    short len;
    char payload[1000];
};

//----------------------------------------------------------------------------
// 데이터를 랜덤 생성 함수
//----------------------------------------------------------------------------
void makeRandData()
{

}

//----------------------------------------------------------------------------
// Enqueue (Producer) 스레드 함수
//----------------------------------------------------------------------------
void ProducerThread()
{
    printf(">> Producer 스레드 시작: 총 %d개의 Int 데이터 전송 예정\n", TEST_COUNT);

    for (int i = 1; i <= TEST_COUNT; ++i)
    {
        // 1. 전송할 데이터 (순서 확인을 위해 i 값을 사용)
        char data_to_send = i;

        // 2. 링 버퍼에 Enqueue (Enqueue 함수 내부에서 블록킹 발생 가능)
        int enqueued_size = g_buffer.Enqueue(&data_to_send, DATA_SIZE);

        if (enqueued_size == DATA_SIZE)
        {
            g_enq_count++;
            g_total_sent_value += data_to_send;
        }
        else
        {
            // 이 블록킹 Enqueue 구현에서는 이 분기가 거의 발생하지 않음
            printf("[Producer] Enqueue 실패 (작게 들어감): 요청 %d, 실제 %d\n", DATA_SIZE, enqueued_size);
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