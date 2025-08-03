// CRingBuffer.cpp
#include "CRingBuffer.h"
#include <cstring>   // memcpy
#include <algorithm> // std::min

CRingBuffer::CRingBuffer(void)
    : m_pBuffer(nullptr), m_iBufferSize(0), m_iFront(0), m_iRear(0), m_bIsFull(false)
{
}

CRingBuffer::CRingBuffer(int iBufferSize)
    : m_pBuffer(new char[iBufferSize]), m_iBufferSize(iBufferSize), m_iFront(0), m_iRear(0), m_bIsFull(false)
{
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

int CRingBuffer::Enqueue(const char* chpData, int iSize)
{
    if (iSize <= 0 || chpData == nullptr || m_iBufferSize == 0)
        return 0;

    int freeSize = GetFreeSize();
    if (iSize > freeSize)
        iSize = freeSize;

    if (iSize == 0)
    {
        return 0;
    }

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
    m_bIsFull = (m_iRear == m_iFront);

    return iSize;
}

int CRingBuffer::Dequeue(char* chpDest, int iSize)
{
    if (iSize <= 0 || chpDest == nullptr || m_iBufferSize == 0)
        return 0;

    int useSize = GetUseSize();
    if (iSize > useSize)
        iSize = useSize;

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
    if (iSize > useSize)
        iSize = useSize;

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
        return m_iBufferSize - m_iRear - (m_iFront == 0 ? 0 : 1);
    else
        return m_iFront - m_iRear - 1;
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

    int freeSize = GetFreeSize();
    if (iSize > freeSize)
        iSize = freeSize;

    m_iRear = (m_iRear + iSize) % m_iBufferSize;
    m_bIsFull = (m_iRear == m_iFront);

    return iSize;
}

int CRingBuffer::MoveFront(int iSize)
{
    if (m_iBufferSize == 0)
        return 0;

    int useSize = GetUseSize();
    if (iSize > useSize)
        iSize = useSize;

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
