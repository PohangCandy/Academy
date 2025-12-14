// CRingBuffer.cpp
#include "CRingBuffer.h"
//#include <cstring>   // memcpy
//#include <algorithm> // std::min


CRingBuffer::CRingBuffer(void)
    : m_pBuffer(nullptr), m_iBufferSize(0), m_iFront(0), m_iRear(0), m_bIsFull(false)
{
    InitializeCriticalSection(&m_csRingbuffer);
}

CRingBuffer::CRingBuffer(int iBufferSize)
    : m_pBuffer(new char[iBufferSize]), m_iBufferSize(iBufferSize), m_iFront(0), m_iRear(0), m_bIsFull(false)
{
    InitializeCriticalSection(&m_csRingbuffer);
}

CRingBuffer::~CRingBuffer() {
    if (m_pBuffer != nullptr) {
        delete[] m_pBuffer; // 할당된 메모리 해제
        m_pBuffer = nullptr;
    }
    DeleteCriticalSection(&m_csRingbuffer);
}

void CRingBuffer::Resize(int size)
{
    delete[] m_pBuffer;
    m_pBuffer = new char[size];
    m_iBufferSize = size;
    m_iFront = 0;
    m_iRear = 0;
    m_bIsFull = false;
}

int CRingBuffer::GetBufferSize(void)
{
    return m_iBufferSize;
}

int CRingBuffer::GetUseSize(void)
{
    if (m_iBufferSize == 0) return 0;

    if (m_bIsFull)
        return m_iBufferSize;

    if (m_iRear >= m_iFront)
        return m_iRear - m_iFront;
    else
        return m_iBufferSize - (m_iFront - m_iRear);
}

int CRingBuffer::GetFreeSize(void)
{
    if (m_iBufferSize == 0) return 0;
    return m_iBufferSize - GetUseSize();
}

//---------------------------------------------------------------------
// 수신 링버퍼에서 Enqueue를 사용하는 상황은 없음.
// 수신 링버퍼가 꽉 차는 상황은 정상적인 경우 발생할 수 없음.
// Recv를 버퍼에 거쳐서 실행하지 않고 바로 수신 링버퍼에 담을 것
// 
// 송신 링버퍼에 Enqueue할때만 사용
//---------------------------------------------------------------------
int CRingBuffer::Enqueue(const char* chpData, int iSize)
{
    if (iSize <= 0 || chpData == nullptr || m_iBufferSize == 0)
        return 0;

    //---------------------------------------
    // 연결이 많다면 끊어줘야 함
    // 그냥 0 리턴하게 만들어도 될 듯
    // 링버퍼 사용자가 링버퍼에 대입한 값과 다르게 나온 경우에 대해서 알아서 처리하도록 만들어줘야 함.
    //---------------------------------------
    int freeSize = GetFreeSize();
    if (iSize > freeSize)
        //iSize = freeSize;
        return 0;

    if (iSize == 0)
    {
        return 0;
    }

    //-----------------------------------------------
    // tailsize : rear뒤에 오는 공간의 크기
    // m_iRear : 1, m_iFront : 5라고 하더라도, tailsize 계산방식은 문제없음.
    // 해당 경우 남은 공간인 fresssize보다 작은 크기만큼만 받을 것임.
    // -> 애초에 해당 공간보다 작은 메시지만 들어오는 상황이 정상임.
    //-----------------------------------------------
    int tailSize = m_iBufferSize - m_iRear;
    if (tailSize >= iSize)
    {
        memcpy(m_pBuffer + m_iRear, chpData, iSize);
    }
    else
    {
        memcpy(m_pBuffer + m_iRear, chpData, tailSize);
        memcpy(m_pBuffer, chpData + tailSize, iSize - tailSize);
    }

    m_iRear = (m_iRear + iSize) % m_iBufferSize;
    //--------------------------------------------------------------------
    // Rear와 Front가 같아지는 경우 동기화가 필요해진다.
    // 나중에 동기화를 없애는 형태로 제작하기 위해서 둘은 구분되어져야 한다.
    //--------------------------------------------------------------------
    m_bIsFull = (m_iRear == m_iFront);

    return iSize;
}

int CRingBuffer::Dequeue(char* chpDest, int iSize)
{
    if (iSize <= 0 || chpDest == nullptr || m_iBufferSize == 0)
        return 0;

    //------------------------------------------
    // Dequeue했을 때, iSize길이보다 짧은 길이가 리턴되는 경우
    // 애초에 Peek를 거쳐서 메시지 길이만큼 있는지 확인함.
    // Dequeue하는 경우는, 메시지 길이만큼 읽고, 해당 크기만큼 Dequeue시도
    // 
    // 메시지 길이만큼 읽으려고 했는데, 없는 경우
    // 

    // 
    // 읽어야 하는 길이만큼 메시지가 들어오지 않은 경우
    // 1. 수신 링버퍼에 아직 메시지가 다 도착하지 않았고, 앞단에 찌꺼기 만큼만 있는 경우
    // 2. 송신 링버퍼엔 그런 상황이 발생할 수 없음.
    // 
    // 수신 링버퍼에 메시지 길이만큼의 길이가 있는가?
    //   Y : 있다면, 그만큼 Dequeue해서 메시지를 스위치 문으로 처리
    //   N : 없으면 다시 1번으로. 읽혀지면 안되므로 그냥 0리턴
    //   다시 Recv로 링버퍼에 데이터 넣기 반복
    //------------------------------------------
    int useSize = GetUseSize();
    if (iSize > useSize)
        iSize = 0;

    if (iSize == 0)
        return 0;

    int tailSize = m_iBufferSize - m_iFront;
    if (tailSize >= iSize)
    {
        memcpy(chpDest, m_pBuffer + m_iFront, iSize);
    }
    else
    {
        memcpy(chpDest, m_pBuffer + m_iFront, tailSize);
        memcpy(chpDest + tailSize, m_pBuffer, iSize - tailSize);
    }

    m_iFront = (m_iFront + iSize) % m_iBufferSize;
    m_bIsFull = false;

    return iSize;
}

int CRingBuffer::Peek(char* chpDest, int iSize)
{
    if (iSize <= 0 || chpDest == nullptr || m_iBufferSize == 0)
        return 0;


    int useSize = GetUseSize();
    //주어진 메시지 길이만큼 읽을 데이터가 없는 경우
    //수신 링버퍼 -> 아직 메시지가 다 도착하지 않은 경우
    // 만약 클라가 악의적으로 메시지 형식과 다른 메시지를 보냈다면?
    // ex) 1byte짜리 상관없는 메시지가 계속 보내질 경우
    // 서버는 이게 잘못된 메시지인지, 여전히 수신해야하는 메시지인지 어떻게 판단할 수 있지?
    // 일반적으로 메시지에 타입을 넣어야하지 않을까? -> 기준이 되는 타입이 있어야 이런 상황을 판단할 수 있을 듯.
    // 그냥 무조건 메시지 길이 + 데이터로만 받는다? 이상함.
    // 
    //송신 링버퍼 -> 그런 상황이 발생할 수 없음.
    // 사용자가 직접 메시지 형식을 작성 후 다음, 링버퍼에 담아서 보내는데 이때 길이가 더 짧다?
    // 이런 상황이 발생할 수 있나??
    // 애초에 메시지 길이만큼 담을 수 없으면 아예 담지도 않음. Send했는데 메시지 길이만큼도 없다는건 그냥 보낼게 없는 상태임.
    // 그냥 송신 링버퍼가 0
    // 
    // 아직 메시지 길이만큼도 담지 않은 경우, 처음 접속 시, 데이터를 보내기 전에 Recv가 넌블로킹 소켓에 의해 처리된다면 링버퍼는 비어있는 상태
    if (iSize > useSize)
        iSize = 0;

    if (iSize == 0)
        return 0;

    int tailSize = m_iBufferSize - m_iFront;
    if (tailSize >= iSize)
    {
        memcpy(chpDest, m_pBuffer + m_iFront, iSize);
    }
    else
    {
        memcpy(chpDest, m_pBuffer + m_iFront, tailSize);
        memcpy(chpDest + tailSize, m_pBuffer, iSize - tailSize);
    }

    return iSize;
}

void CRingBuffer::ClearBuffer(void)
{
    m_iFront = 0;
    m_iRear = 0;
    m_bIsFull = false;
}

int CRingBuffer::DirectEnqueueSize(void)
{
    if (m_iBufferSize == 0 || m_bIsFull)
        return 0;

    if (m_iRear >= m_iFront)
        return m_iBufferSize - m_iRear;
    else
        return m_iFront - m_iRear;
}

int CRingBuffer::DirectDequeueSize(void)
{
    if (m_iBufferSize == 0)
        return 0;

    if (m_iFront <= m_iRear)
        return m_iRear - m_iFront;
    else
        return m_iBufferSize - m_iFront;
}

int CRingBuffer::MoveRear(int iSize)
{
    if (m_iBufferSize == 0)
        return 0;

    //---------------------------
    // Rear를 옮기려고 하는데 해당 크기만큼 옮길 수 없는 경우
    // Rear 변화없이 0리턴
    //---------------------------
    int freeSize = GetFreeSize();
    if (iSize > freeSize)
        return 0;

    m_iRear = (m_iRear + iSize) % m_iBufferSize;
    m_bIsFull = (m_iRear == m_iFront);

    return iSize;
}

int CRingBuffer::MoveFront(int iSize)
{
    if (m_iBufferSize == 0)
        return 0;

 //---------------------------
// Front를 옮기려고 하는데 해당 크기만큼 옮길 수 없는 경우
// Front 변화없이 0리턴
//---------------------------
    int useSize = GetUseSize();
    if (iSize > useSize)
        return 0;

    m_iFront = (m_iFront + iSize) % m_iBufferSize;
    m_bIsFull = false;


    return iSize;
}

char* CRingBuffer::GetFrontBufferPtr(void)
{
    if (m_iBufferSize == 0)
        return nullptr;
    return m_pBuffer + m_iFront;
}

char* CRingBuffer::GetRearBufferPtr(void)
{
    if (m_iBufferSize == 0)
        return nullptr;
    return m_pBuffer + m_iRear;
}

void CRingBuffer::GetLockBuffer()
{
    EnterCriticalSection(&m_csRingbuffer);
}

void CRingBuffer::UnLockBuffer()
{
    LeaveCriticalSection(&m_csRingbuffer);
}
