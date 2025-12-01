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
// 메시지를 랜덤으로 생성해서 넣고 뺀다.
// 
// 결론 : 인 큐, 디 큐 1개씩 했을 때 별 문제없이 잘 동작함.
// 인큐가 실패할 때가 있는데, 그건 다른 스레드가 디큐하기 전에 링버퍼가 꽉 차서 못넣은거라 신경 안 떠도 됨.
// 결국 아예 인큐 조차 못하는 상황이 생길 수 있으나, 인큐 한 데이터에 대해선 디큐를 잘 하고 있음.
//---------------------------------------------------------------------------------------------
#include <iostream>
#include <thread>
#include <string>
#include <chrono>
#include "TestUtils.h"
#include "CRingBuffer.h" // 위에 수정한 헤더 파일\

using namespace std;

// 테스트용 상수
const int BUFFER_SIZE = 100;
const int TEST_COUNT = 1000;
const int MSG_SIZE = 6;

//dequeue 할 길이를 알아야하므로 메시지 길이가 앞에 헤더 작성 필요.
struct Msg {
    int len = 0;
    //memcpy 목적 주소가 되려면 string이 아닌 char*이어야 함.
    char payload[MSG_SIZE];
};


//--------------------------------------------------
// Enqueue, Dequeue할 데이터를 담을 자료구조
//--------------------------------------------------
Msg g_EnqueData[TEST_COUNT];
Msg g_DequeData[TEST_COUNT];
int g_WrongNum[TEST_COUNT];
Msg g_WrongData[TEST_COUNT][2];


//--------------------------------------------------
// 링버퍼와 enq, deq 횟수 저장할 변수
//--------------------------------------------------
CRingBuffer g_buffer(BUFFER_SIZE);
int g_enq_count(0);
int g_deq_count(0);



//----------------------------------------------------------------------------
// 메시지 랜덤 생성 함수
//----------------------------------------------------------------------------
Msg makeRandMsg()
{
    Msg m;

    int len = GetRandomNumber(1, MSG_SIZE - sizeof(int));
    string enqueueData = GetRandomString(len);

    m.len = len;
    m.payload = enqueueData;
    //strcpy_s(m.payload, MSG_SIZE, enqueueData.c_str());
    return m;
}

//----------------------------------------------------------------------------
// Enqueue (Producer) 스레드 함수
//----------------------------------------------------------------------------
void ProducerThread()
{
    printf(">> Producer 스레드 시작: 총 %d개의 Int 데이터 전송 예정\n", TEST_COUNT);

    while(1)
    {
        //---------------------------------------------------
        // 1. 전송할 데이터
        // 버퍼 사이즈보다 작은 값 중에 랜덤하게 골라서
        // 메시지 길이와 메시지를 string 형태로 enqueue
        //---------------------------------------------------
        Msg sendMsg = makeRandMsg();

        int lenSize = sizeof(sendMsg.len);
        int StringSize = sendMsg.len;

        //메시지 길이 = 메시지 헤더 크기 + 메시지 크기
        //메시지 헤더 길이 = 메시지 길이
        int enqueued_size = g_buffer.Enqueue((char*)&sendMsg, lenSize + StringSize);

        if (enqueued_size == lenSize + StringSize)
        {
            g_EnqueData[g_enq_count] = sendMsg;
            g_enq_count++;
            if (g_enq_count == TEST_COUNT)
                break;
        }
        else
        {
            // 이 블록킹 Enqueue 구현에서는 이 분기가 거의 발생하지 않음
            printf("[Producer] Enqueue 실패 (작게 들어감): 요청 %d, 실제 %d\n", sizeof(sendMsg), enqueued_size);
        }

        //// 아주 짧은 sleep으로 스케줄링 전환 유도
        //std::this_thread::sleep_for(std::chrono::microseconds(1));
    }
    printf("<< Producer 스레드 종료: 총 %d개 전송 완료\n", (int)g_enq_count);
}

//----------------------------------------------------------------------------
// Dequeue (Consumer) 스레드 함수
//----------------------------------------------------------------------------
void ConsumerThread()
{
    printf(">> Consumer 스레드 시작: 총 %d개의 Int 데이터 수신 예정\n", TEST_COUNT);

    

    while (1)
    {
        //recv할때마다 초기화
        Msg recvMsg;

        //------------------------------------------------------
        // 1. 링 버퍼에서 Dequeue
        // 먼저 메시지 길이만큼 읽을 후, 해당 메시지 길이를 Dequeue
        //------------------------------------------------------
        int Header_size = g_buffer.Peek((char*)&recvMsg.len, sizeof(recvMsg.len));
        if (Header_size == sizeof(recvMsg.len) && recvMsg.len != 0)
        {
            if (g_buffer.GetUseSize() >= recvMsg.len + sizeof(recvMsg.len))
            {
                //이미 읽은 헤더는 제외하고 읽자.
                g_buffer.MoveFront(sizeof(recvMsg.len));
                int dequeued_size = g_buffer.Dequeue((char*)&recvMsg.payload, recvMsg.len);

                if (dequeued_size == recvMsg.len)
                {
                    g_DequeData[g_deq_count] = recvMsg;
                    g_deq_count++;

                    if (g_deq_count == TEST_COUNT)
                        break;
                }
                else
                {

                    printf("[CONSUMER ERROR] 부분 데이터 수신 오류: 요청 %d, 실제 %d\n", sizeof(int) + Header_size, dequeued_size);
                    // return;
                }
            }
        }

        // 아주 짧은 sleep으로 스케줄링 전환 유도
        //std::this_thread::sleep_for(std::chrono::microseconds(2));
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

    int same = 0;
    int dif = 0;

    for (int i = 0; i < TEST_COUNT; i++)
    {
        bool bWrong = false;

        //enqueue 데이터와 dequeue 데이터가 같은지 확인
        if (g_EnqueData[i].len == g_DequeData[i].len)
        {
            int msglength = g_EnqueData[i].len;
            for (int j = 0; j < msglength; j++)
            {
                if (*(g_EnqueData[i].payload + j) != *(g_DequeData[i].payload + j))
                {
                    bWrong = true;
                    break;
                }
            }
        }
        else
        {
            bWrong = true;
        }

        if (bWrong)
        {
            g_WrongNum[dif] = i;
            g_WrongData[dif][0] = g_EnqueData[i];
            g_WrongData[dif][1] = g_DequeData[i];
            cout << "데이터 손실 또는 처리 불일치 발생." << "\n";
            cout << "불일치가 발생한 인덱스 : " << i << "\n";
            string eS;
            string dS;
            for (int i = 0; i < g_EnqueData[i].len ; i++)
            {
                eS += *(g_EnqueData[i].payload + i);
            }
            for (int i = 0; i < g_DequeData[i].len; i++)
            {
                dS += *(g_DequeData[i].payload + i);
            }

            cout << "Enqueue측 데이터 : " << eS << " Dnqueue측 데이터 : " << dS << "\n";
            cout << "\n";
            dif++;
        }
    }

    if (same == dif)
    {
        printf("테스트 통과: 모든 데이터가 성공적으로 처리되었습니다.\n");
    }
    else
    {
        printf("테스트 실패: 데이터 손실 또는 처리 불일치가 1회 이상 발생.\n");
    }

    return 0;
}