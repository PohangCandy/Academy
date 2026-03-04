#include "CPacketRingBuffer.h"
#include "CPacketForMultiThread.h"


CPacketRingBuffer::CPacketRingBuffer(void)
    : _packetBuffer(nullptr), _capacity(0), _front(0), _rear(0), _IsFull(false)
{
    InitializeCriticalSection(&_csRingbuffer);
}

CPacketRingBuffer::CPacketRingBuffer(int iBufferSize)
    : _packetBuffer(new CPacket*[iBufferSize]), _capacity(iBufferSize), _front(0), _rear(0), _IsFull(false)
{
    InitializeCriticalSection(&_csRingbuffer);
}

CPacketRingBuffer::~CPacketRingBuffer() {
    if (_packetBuffer != nullptr) {
        delete[] _packetBuffer; // 할당된 메모리 해제
        _packetBuffer = nullptr;
    }
    DeleteCriticalSection(&_csRingbuffer);
}


int CPacketRingBuffer::GetBufferSize(void)
{
    /*EnterCriticalSection(&_csRingbuffer);
    LeaveCriticalSection(&_csRingbuffer);*/
    return _capacity;
}

int CPacketRingBuffer::GetUseSize(void)
{
    //캐시에 있는 값을 읽어오기 위한 interlock함수
    //EnterCriticalSection(&_csRingbuffer);
    //LeaveCriticalSection(&_csRingbuffer);
    if (_capacity == 0) return 0;

    if (_IsFull)
        return _capacity;

    if (_rear >= _front)
        return _rear - _front;
    else
        return _capacity - (_front - _rear);
}

int CPacketRingBuffer::GetFreeSize(void)
{
   /* EnterCriticalSection(&_csRingbuffer);
    LeaveCriticalSection(&_csRingbuffer);*/
    if (_capacity == 0) return 0;
    return _capacity - GetUseSize();
}

int CPacketRingBuffer::GetFront()
{
    //EnterCriticalSection(&_csRingbuffer);
    //LeaveCriticalSection(&_csRingbuffer);
    return _front;
}

//---------------------------------------------------------------------
// WSATecv에서 수신 링버퍼 포인터를 넣으므로
// 송신 링버퍼에서만 Enqueue 사용
// 
// 빈공간이 있는 경우 패킷 포인터를 담는다.
//---------------------------------------------------------------------
bool CPacketRingBuffer::Enqueue(CPacket* pPacket)
{
    //EnterCriticalSection(&_csRingbuffer);
    if (pPacket == nullptr || _capacity == 0)
    {
        //LeaveCriticalSection(&_csRingbuffer);
        return false;
    }

    //---------------------------------------
    // 연결이 많다면 끊어줘야 함
    // 그냥 0 리턴하게 만들어도 될 듯
    // 링버퍼 사용자가 링버퍼에 대입한 값과 다르게 나온 경우에 대해서 알아서 처리하도록 만들어줘야 함.
    //---------------------------------------
    if (_IsFull)
    {
        //LeaveCriticalSection(&_csRingbuffer);
        return false;
    }

    _packetBuffer[_rear] = pPacket;

    _rear = (_rear + 1) % _capacity;
    //--------------------------------------------------------------------
    // Rear와 Front가 같아지는 경우 동기화가 필요해진다.
    // 나중에 동기화를 없애는 형태로 제작하기 위해서 둘은 구분되어져야 한다.
    //--------------------------------------------------------------------
    _IsFull = (_rear == _front);

    //LeaveCriticalSection(&_csRingbuffer);
    return true;
}

//--------------------------------------------------------------------
//송신 링버퍼가 Dequeue를 하는 순간은 비동기 송신 직전에 wsabuf에 담시 전 뿐임.
//이 작업을 송신 링버퍼용 맴버 함수에 아예 담아서 구현해준다.
//링버퍼 내 모든 버퍼를 wsabuf에 세팅한 후, 담은 개수 돌려주기
// -> 근데 나중에 락프리 큐로 대체한다고 치면 결국 락프리 큐에도 wsabuf에 세팅하는 작업이 들어가야한다??-> 이건 아니지
// 결국 큐에 남아있는 작업들을 꺼내는 작업이 필요함.
// 송신 링버퍼에 남아있는 모든 패킷을 꺼내서 처리하는 식으로 만들면 되지 않을까?
// 이렇게되면 몇 바이트 단위로 꺼내는게 아니라, 패킷 포인터 하나만 꺼내는 식으로 처리한다.
//--------------------------------------------------------------------
bool CPacketRingBuffer::Dequeue(CPacket*& pPacket)
{
    //EnterCriticalSection(&_csRingbuffer);
    if (_capacity == 0)
    {
        //LeaveCriticalSection(&_csRingbuffer);
        return false;
    }

    if (0 >= GetUseSize())
    {
        //LeaveCriticalSection(&_csRingbuffer);
        return false;
    }

    pPacket = _packetBuffer[_front];

    _front = (_front + 1) % _capacity;
    _IsFull = false;

    //LeaveCriticalSection(&_csRingbuffer);
    return true;
}

void CPacketRingBuffer::ClearBuffer(void)
{
    //EnterCriticalSection(&_csRingbuffer);
    _front = 0;
    _rear = 0;
    _IsFull = false;
    //LeaveCriticalSection(&_csRingbuffer);
}

int CPacketRingBuffer::MoveRear(int iSize)
{
    //EnterCriticalSection(&_csRingbuffer);
    if (_capacity == 0)
    {
        //LeaveCriticalSection(&_csRingbuffer);
        return 0;
    }

    //---------------------------
    // Rear를 옮기려고 하는데 해당 크기만큼 옮길 수 없는 경우
    // Rear 변화없이 0리턴
    //---------------------------
    int freeSize = GetFreeSize();
    if (iSize > freeSize)
    {
        //LeaveCriticalSection(&_csRingbuffer);
        return 0;
    }

    _rear = (_rear + iSize) % _capacity;
    _IsFull = (_rear == _front);

    //LeaveCriticalSection(&_csRingbuffer);
    return iSize;
}

int CPacketRingBuffer::MoveFront(int iSize)
{
    //EnterCriticalSection(&_csRingbuffer);
    if (_capacity == 0)
    {
        //LeaveCriticalSection(&_csRingbuffer);
        return 0;
    }

 //---------------------------
// Front를 옮기려고 하는데 해당 크기만큼 옮길 수 없는 경우
// Front 변화없이 0리턴
//---------------------------
    int useSize = GetUseSize();
    if (iSize > useSize)
    {
        //LeaveCriticalSection(&_csRingbuffer);
        return 0;
    }

    _front = (_front + iSize) % _capacity;
    _IsFull = false;

    //LeaveCriticalSection(&_csRingbuffer);
    return iSize;
}

CPacket** CPacketRingBuffer::GetFrontBufferPtr(void)
{
    //EnterCriticalSection(&_csRingbuffer);
    //LeaveCriticalSection(&_csRingbuffer);
    if (_capacity == 0)
        return nullptr;
    return _packetBuffer + _front;
}

CPacket** CPacketRingBuffer::GetRearBufferPtr(void)
{
    //EnterCriticalSection(&_csRingbuffer);
    //LeaveCriticalSection(&_csRingbuffer);
    if (_capacity == 0)
        return nullptr;
    return _packetBuffer + _rear;
}

CPacket** CPacketRingBuffer::GetBufPtr(void)
{
    //EnterCriticalSection(&_csRingbuffer);
    //LeaveCriticalSection(&_csRingbuffer);
    return _packetBuffer;
}


//---------------------------------------
// Resize는 아무것도 없을때 키우는게 아니라 기존의 메모리는 복사해서 저장해두고, 
//---------------------------------------
//void CPacketRingBuffer::Resize(int size)
//{
//    delete[] m_packetBuffer;
//    m_packetBuffer = new CPacket* [size];
//    m_iBufferSize = size;
//    m_iFront = 0;
//    m_iRear = 0;
//    m_bIsFull = false;
//}

//----------------------------------------------
//송신링버퍼에서 Peek를 쓸 일은 없음.
//----------------------------------------------
//int CPacketRingBuffer::Peek(CPacket* pPacket, int iSize)
//{
//    EnterCriticalSection(&m_csRingbuffer);
//    if (iSize <= 0 || pPacket == nullptr || m_iBufferSize == 0)
//        return 0;
//
//
//    int useSize = GetUseSize();
//    //주어진 메시지 길이만큼 읽을 데이터가 없는 경우
//    //수신 링버퍼 -> 아직 메시지가 다 도착하지 않은 경우
//    // 만약 클라가 악의적으로 메시지 형식과 다른 메시지를 보냈다면?
//    // ex) 1byte짜리 상관없는 메시지가 계속 보내질 경우
//    // 서버는 이게 잘못된 메시지인지, 여전히 수신해야하는 메시지인지 어떻게 판단할 수 있지?
//    // 일반적으로 메시지에 타입을 넣어야하지 않을까? -> 기준이 되는 타입이 있어야 이런 상황을 판단할 수 있을 듯.
//    // 그냥 무조건 메시지 길이 + 데이터로만 받는다? 이상함.
//    // 
//    //송신 링버퍼 -> 그런 상황이 발생할 수 없음.
//    // 사용자가 직접 메시지 형식을 작성 후 다음, 링버퍼에 담아서 보내는데 이때 길이가 더 짧다?
//    // 이런 상황이 발생할 수 있나??
//    // 애초에 메시지 길이만큼 담을 수 없으면 아예 담지도 않음. Send했는데 메시지 길이만큼도 없다는건 그냥 보낼게 없는 상태임.
//    // 그냥 송신 링버퍼가 0
//    // 
//    // 아직 메시지 길이만큼도 담지 않은 경우, 처음 접속 시, 데이터를 보내기 전에 Recv가 넌블로킹 소켓에 의해 처리된다면 링버퍼는 비어있는 상태
//    if (iSize > useSize)
//        iSize = 0;
//
//    if (iSize == 0)
//        return 0;
//
//    int tailSize = m_iBufferSize - m_iFront;
//    if (tailSize >= iSize)
//    {
//        memcpy(chpDest, m_packetBuffer + m_iFront, iSize);
//    }
//    else
//    {
//        memcpy(chpDest, m_packetBuffer + m_iFront, tailSize);
//        memcpy(chpDest + tailSize, m_packetBuffer, iSize - tailSize);
//    }
//
//    return iSize;
//}

//int CPacketRingBuffer::DirectEnqueueSize(void)
//{
//    if (_capacity == 0 || _IsFull)
//        return 0;
//
//    if (_rear >= _front)
//        return _capacity - _rear;
//    else
//        return _front - _rear;
//}
//
//int CPacketRingBuffer::DirectDequeueSize(void)
//{
//    if (_capacity == 0)
//        return 0;
//
//    if (_front <= _rear)
//        return _rear - _front;
//    else
//        return _capacity - _front;
//}