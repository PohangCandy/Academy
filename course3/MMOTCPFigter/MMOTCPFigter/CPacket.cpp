#include "CPacket.h"
#include <cstring>   // std::memcpy
#include <algorithm> // std::max
#include <new>       // std::nothrow

// ============================== 내부 유틸 ==============================

//void CPacket::_EnsureCapacity(int requireBytes)
//{
//    // requireBytes는 추가로 "더 써야 하는" 용량.
//    // 필요 총량 = m_iWritePos + requireBytes
//    if (m_iWritePos + requireBytes <= m_iBufferSize) return;
//
//    int newSize = std::max(m_iBufferSize, 1);
//    while (newSize < m_iWritePos + requireBytes)
//        newSize <<= 1; // 2배씩 확장
//
//    char* newBuf = new (std::nothrow) char[newSize];
//    if (!newBuf) return; // 메모리 부족 시 안전 탈출(실전이면 예외/로그 권장)
//
//    // 기존 데이터 복사
//    if (m_chpBuffer && m_iWritePos > 0)
//        std::memcpy(newBuf, m_chpBuffer, m_iWritePos);
//
//    delete[] m_chpBuffer;
//    m_chpBuffer = newBuf;
//    m_iBufferSize = newSize;
//}

void CPacket::_CompactIfEmpty()
{
    // 모든 데이터를 읽어버렸다면 포인터를 0으로 초기화
    if (m_iReadPos == m_iWritePos)
    {
        m_iReadPos = 0;
        m_iWritePos = 0;
        m_iDataSize = 0;
    }
    else
    {
        // 일반적으로는 그대로 둔다. (불필요한 memmove를 피함)
        // 필요 시 여기서 앞당기는 최적화도 가능:
        // if (m_iReadPos > some_threshold) { memmove(0); }
    }
}

// ============================== 생성/소멸/관리 ==============================

CPacket::CPacket()
{
    m_iBufferSize = eBUFFER_DEFAULT;
    //m_chpBuffer = new char[m_iBufferSize];
    m_iDataSize = 0;
    m_iReadPos = 0;
    m_iWritePos = 0;
}

CPacket::CPacket(int iBufferSize)
{
    if (iBufferSize <= 0) iBufferSize = eBUFFER_DEFAULT;
    m_iBufferSize = iBufferSize;
    //m_chpBuffer = new char[m_iBufferSize];
    m_iDataSize = 0;
    m_iReadPos = 0;
    m_iWritePos = 0;
}

CPacket::~CPacket()
{
    CPacket::Clear();
}

void CPacket::Clear(void)
{
    m_iReadPos = 0;
    m_iWritePos = 0;
    m_iDataSize = 0;
    // 버퍼 내용은 굳이 지우지 않음(성능). 민감하면 memset.
}

// ============================== Move Pos ==============================

int CPacket::MoveWritePos(int iSize)
{
    if (iSize <= 0) return 0;
    //_EnsureCapacity(iSize);
    int moved = iSize;
    m_iWritePos += moved;
    m_iDataSize = m_iWritePos - m_iReadPos;
    return moved;
}

int CPacket::MoveReadPos(int iSize)
{
    if (iSize <= 0) return 0;
    // 읽을 수 있는 만큼만 이동
    int canMove = m_iWritePos - m_iReadPos;
    if (iSize > canMove) iSize = canMove;
    m_iReadPos += iSize;
    m_iDataSize = m_iWritePos - m_iReadPos;
    _CompactIfEmpty();
    return iSize;
}

// ============================== 대입 연산 ==============================

CPacket& CPacket::operator = (CPacket& src)
{
    if (this == &src) return *this;

    // 버퍼 크기 맞추기
    if (m_iBufferSize < src.m_iWritePos)
    {
        //delete[] m_chpBuffer;
        m_iBufferSize = std::max(src.m_iBufferSize, src.m_iWritePos);
        //m_chpBuffer = new char[m_iBufferSize];
    }

    // 데이터 복사(버퍼 전체가 아니라 실제 writePos까지)
    if (src.m_iWritePos > 0)
        std::memcpy(m_chpBuffer, src.m_chpBuffer, src.m_iWritePos);

    m_iReadPos = src.m_iReadPos;
    m_iWritePos = src.m_iWritePos;
    m_iDataSize = src.m_iDataSize;

    return *this;
}

// ============================== Raw Put/Get ==============================

int CPacket::PutData(char* chpSrc, int iSrcSize)
{
    if (iSrcSize <= 0 || chpSrc == nullptr) return 0;

    //_EnsureCapacity(iSrcSize);
    std::memcpy(m_chpBuffer + m_iWritePos, chpSrc, iSrcSize);
    m_iWritePos += iSrcSize;
    m_iDataSize = m_iWritePos - m_iReadPos;
    return iSrcSize;
}

int CPacket::GetData(char* chpDest, int iSize)
{
    if (iSize <= 0 || chpDest == nullptr) return 0;

    int available = m_iWritePos - m_iReadPos;
    if (available <= 0) return 0;

    int toCopy = (iSize < available) ? iSize : available;
    std::memcpy(chpDest, m_chpBuffer + m_iReadPos, toCopy);
    m_iReadPos += toCopy;
    m_iDataSize = m_iWritePos - m_iReadPos;
    _CompactIfEmpty();
    return toCopy;
}

// ============================== << Serialize ==============================

CPacket& CPacket::operator << (unsigned char byValue)
{
    PutData(reinterpret_cast<char*>(&byValue), sizeof(byValue));
    return *this;
}
CPacket& CPacket::operator << (char chValue)
{
    PutData(reinterpret_cast<char*>(&chValue), sizeof(chValue));
    return *this;
}
CPacket& CPacket::operator << (short shValue)
{
    PutData(reinterpret_cast<char*>(&shValue), sizeof(shValue));
    return *this;
}
CPacket& CPacket::operator << (unsigned short wValue)
{
    PutData(reinterpret_cast<char*>(&wValue), sizeof(wValue));
    return *this;
}
CPacket& CPacket::operator << (int iValue)
{
    PutData(reinterpret_cast<char*>(&iValue), sizeof(iValue));
    return *this;
}
CPacket& CPacket::operator << (long lValue)
{
    PutData(reinterpret_cast<char*>(&lValue), sizeof(lValue));
    return *this;
}
CPacket& CPacket::operator << (float fValue)
{
    PutData(reinterpret_cast<char*>(&fValue), sizeof(fValue));
    return *this;
}
CPacket& CPacket::operator << (__int64 iValue)
{
    PutData(reinterpret_cast<char*>(&iValue), sizeof(iValue));
    return *this;
}
CPacket& CPacket::operator << (double dValue)
{
    PutData(reinterpret_cast<char*>(&dValue), sizeof(dValue));
    return *this;
}

// ============================== >> Deserialize ==============================

CPacket& CPacket::operator >> (char& chValue)
{
    GetData(reinterpret_cast<char*>(&chValue), sizeof(chValue));
    return *this;
}
CPacket& CPacket::operator >> (unsigned char& byValue)
{
    GetData(reinterpret_cast<char*>(&byValue), sizeof(byValue));
    return *this;
}
CPacket& CPacket::operator >> (short& shValue)
{
    GetData(reinterpret_cast<char*>(&shValue), sizeof(shValue));
    return *this;
}
CPacket& CPacket::operator >> (unsigned short& wValue)
{
    GetData(reinterpret_cast<char*>(&wValue), sizeof(wValue));
    return *this;
}
CPacket& CPacket::operator >> (int& iValue)
{
    GetData(reinterpret_cast<char*>(&iValue), sizeof(iValue));
    return *this;
}
CPacket& CPacket::operator >> (unsigned int& dwValue)
{
    GetData(reinterpret_cast<char*>(&dwValue), sizeof(dwValue));
    return *this;
}
CPacket& CPacket::operator >> (float& fValue)
{
    GetData(reinterpret_cast<char*>(&fValue), sizeof(fValue));
    return *this;
}
CPacket& CPacket::operator >> (__int64& iValue)
{
    GetData(reinterpret_cast<char*>(&iValue), sizeof(iValue));
    return *this;
}
CPacket& CPacket::operator >> (double& dValue)
{
    GetData(reinterpret_cast<char*>(&dValue), sizeof(dValue));
    return *this;
}
