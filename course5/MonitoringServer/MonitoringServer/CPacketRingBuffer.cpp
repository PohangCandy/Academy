#include "CPacketRingBuffer.h"
#include "CPacketForMultiThread.h"

CPacketRingBuffer::CPacketRingBuffer(void)
    : _packetBuffer(nullptr), _capacity(0), _front(0), _rear(0), _IsFull(false)
{
    InitializeCriticalSection(&_csRingbuffer);
}

CPacketRingBuffer::CPacketRingBuffer(int iBufferSize)
    : _packetBuffer(new CPacket* [iBufferSize]), _capacity(iBufferSize), _front(0), _rear(0), _IsFull(false)
{
    InitializeCriticalSection(&_csRingbuffer);
}

CPacketRingBuffer::~CPacketRingBuffer() {
    if (_packetBuffer != nullptr) {
        delete[] _packetBuffer;
        _packetBuffer = nullptr;
    }
    DeleteCriticalSection(&_csRingbuffer);
}

int CPacketRingBuffer::GetBufferSize(void) { return _capacity; }

int CPacketRingBuffer::GetUseSize(void)
{
    if (_capacity == 0) return 0;
    if (_IsFull) return _capacity;
    if (_rear >= _front) return _rear - _front;
    else return _capacity - (_front - _rear);
}

int CPacketRingBuffer::GetFreeSize(void)
{
    if (_capacity == 0) return 0;
    return _capacity - GetUseSize();
}

int CPacketRingBuffer::GetFront() { return _front; }

bool CPacketRingBuffer::Enqueue(CPacket* pPacket)
{
    if (pPacket == nullptr || _capacity == 0) return false;
    if (_IsFull) return false;

    _packetBuffer[_rear] = pPacket;
    _rear = (_rear + 1) % _capacity;
    _IsFull = (_rear == _front);
    return true;
}

bool CPacketRingBuffer::Dequeue(CPacket*& pPacket)
{
    if (_capacity == 0) return false;
    if (0 >= GetUseSize()) return false;

    pPacket = _packetBuffer[_front];
    _front = (_front + 1) % _capacity;
    _IsFull = false;
    return true;
}

void CPacketRingBuffer::ClearBuffer(void)
{
    _front = 0;
    _rear = 0;
    _IsFull = false;
}

int CPacketRingBuffer::MoveRear(int iSize)
{
    if (_capacity == 0) return 0;
    int freeSize = GetFreeSize();
    if (iSize > freeSize) return 0;

    _rear = (_rear + iSize) % _capacity;
    _IsFull = (_rear == _front);
    return iSize;
}

int CPacketRingBuffer::MoveFront(int iSize)
{
    if (_capacity == 0) return 0;
    int useSize = GetUseSize();
    if (iSize > useSize) return 0;

    _front = (_front + iSize) % _capacity;
    _IsFull = false;
    return iSize;
}

CPacket** CPacketRingBuffer::GetFrontBufferPtr(void)
{
    if (_capacity == 0) return nullptr;
    return _packetBuffer + _front;
}

CPacket** CPacketRingBuffer::GetRearBufferPtr(void)
{
    if (_capacity == 0) return nullptr;
    return _packetBuffer + _rear;
}

CPacket** CPacketRingBuffer::GetBufPtr(void)
{
    return _packetBuffer;
}
