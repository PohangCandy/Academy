#include "CPacketForMultiThread.h"
#include "CommonProtocol.h"

// ============================== 내부 유틸 ==============================

void CPacket::_EnsureCapacity(int requireBytes)
{
    if (m_iWritePos + requireBytes <= m_iBufferSize) return;

    int newSize = max(m_iBufferSize, 1);
    while (newSize < m_iWritePos + requireBytes)
        newSize <<= 1;

    char* newBuf = new (std::nothrow) char[newSize];
    if (!newBuf)
    {
        printf("[CPacket] buffer expansion failed\n");
        return;
    }

    if (m_chpBuffer && m_iWritePos > 0)
        std::memcpy(newBuf, m_chpBuffer, m_iWritePos);

    delete[] m_chpBuffer;
    m_chpBuffer = newBuf;
    m_iBufferSize = newSize;
}

void CPacket::_CompactIfEmpty()
{
    if (m_iReadPos == m_iWritePos)
    {
        m_iReadPos = 0;
        m_iWritePos = 0;
        m_iDataSize = 0;
    }
}

//--------------------------------------------------------------
// NET 인코딩 (WAN용 - 암호화 헤더 5바이트)
//
// [FIX] 기존 코드 버그: m_chpBuffer[1] = (short)payLoadSize
//       char에 short를 대입하면 상위 바이트가 잘림 (256 이상 페이로드 시 오류)
//       수정: *(unsigned short*)&m_chpBuffer[1] = (unsigned short)payLoadSize
//--------------------------------------------------------------
void CPacket::EncodeForNet(unsigned char packetCode, unsigned char packetKey)
{
    if (IsEncoded) return;

    if (_MsgheaderSize == -1)
    {
        printf("[CPacket/EncodeForNet] header size not set\n");
        __debugbreak();
    }

    unsigned char checksum = 0;
    unsigned char randkey = rand() % 100;
    unsigned char paraP = 0;
    unsigned char encodeP = 0;

    int payLoadSize = m_iDataSize - _MsgheaderSize;

    // 체크섬 계산
    for (int i = 0; i < payLoadSize; i++)
    {
        char* pPacketChar = &m_chpBuffer[_MsgheaderSize + i];
        checksum += *pPacketChar % 256;
    }
    m_chpBuffer[4] = checksum % 256;

    // XOR 암호화 (체크섬 + 페이로드)
    for (int i = 0; i < payLoadSize + (int)sizeof(checksum); i++)
    {
        char* pPacketChar = &m_chpBuffer[_MsgheaderSize + i - sizeof(checksum)];
        paraP = *pPacketChar ^ (paraP + randkey + (i + 1));
        *pPacketChar = paraP ^ (encodeP + packetKey + (i + 1));
        encodeP = *pPacketChar;
    }

    // 헤더 기록
    m_chpBuffer[0] = packetCode;
    // [FIX] WORD(2바이트) 단위로 기록 — 기존 코드는 char 1바이트만 기록하여 256 이상 길이에서 잘렸음
    *(unsigned short*)&m_chpBuffer[1] = (unsigned short)payLoadSize;
    m_chpBuffer[3] = randkey;

    IsEncoded = true;
}

//--------------------------------------------------------------
// LAN 인코딩 (내부용 - 단순 WORD Len 헤더 2바이트, 암호화 없음)
//--------------------------------------------------------------
void CPacket::EncodeForLan()
{
    if (IsEncoded) return;

    if (_MsgheaderSize == -1)
    {
        printf("[CPacket/EncodeForLan] header size not set\n");
        __debugbreak();
    }

    unsigned short payLoadSize = (unsigned short)(m_iDataSize - _MsgheaderSize);
    *(unsigned short*)m_chpBuffer = payLoadSize;

    IsEncoded = true;
}

//--------------------------------------------------------------
// NET 디코딩 (WAN용)
//--------------------------------------------------------------
bool CPacket::DecodeForNet(PacketHeader* pHeader, unsigned char packetKey)
{
    unsigned char* payload = (unsigned char*)m_chpBuffer + m_iReadPos;
    unsigned int checksum = 0;
    unsigned char beforeparaP = 0;
    unsigned char afterparaP = 0;
    unsigned char encodeP = 0;

    int payLoadSize = pHeader->Len;
    unsigned char* pPacketChar = &pHeader->CheckSum;

    afterparaP = *pPacketChar ^ (encodeP + packetKey + 1);
    encodeP = *pPacketChar;
    *pPacketChar = afterparaP ^ (beforeparaP + pHeader->RandKey + 1);
    beforeparaP = afterparaP;

    for (int i = 0; i < payLoadSize; i++)
    {
        pPacketChar = &payload[i];

        afterparaP = *pPacketChar ^ (encodeP + packetKey + (i + 2));
        encodeP = *pPacketChar;
        *pPacketChar = afterparaP ^ (beforeparaP + pHeader->RandKey + (i + 2));
        beforeparaP = afterparaP;

        checksum += *pPacketChar % 256;
        checksum %= 256;
    }

    if (pHeader->CheckSum != checksum)
    {
        return false;
    }

    return true;
}

// ============================== 생성/소멸/대입 ==============================

CPacket::CPacket()
{
    m_iBufferSize = eBUFFER_DEFAULT;
    m_chpBuffer = new char[m_iBufferSize];
    m_iDataSize = 0;
    m_iReadPos = 0;
    m_iWritePos = 0;
}

CPacket::CPacket(int iBufferSize)
{
    if (iBufferSize <= 0) iBufferSize = eBUFFER_DEFAULT;
    m_iBufferSize = iBufferSize;
    m_chpBuffer = new char[m_iBufferSize];
    m_iDataSize = 0;
    m_iReadPos = 0;
    m_iWritePos = 0;
}

CPacket::~CPacket()
{
    delete[] m_chpBuffer;
    m_chpBuffer = nullptr;
}

void CPacket::Clear(void)
{
    IsEncoded = false;
    m_iReadPos = 0;
    m_iWritePos = 0;
    m_iDataSize = 0;
    mRefCount = 1;	// Alloc한 소유자의 참조 (실무 규칙: Alloc = refCount 1)

    // 팽창된 내부 버퍼를 기본 크기로 복원 (메모리 낭비 방지)
    // _EnsureCapacity로 커진 버퍼가 풀에 영구히 남는 것을 방지
    if (m_iBufferSize > eBUFFER_DEFAULT)
    {
        delete[] m_chpBuffer;
        m_chpBuffer = new char[eBUFFER_DEFAULT];
        m_iBufferSize = eBUFFER_DEFAULT;
    }
}

// ============================== Move Pos ==============================

int CPacket::MoveWritePos(int iSize)
{
    if (iSize <= 0) return 0;
    _EnsureCapacity(iSize);
    m_iWritePos += iSize;
    m_iDataSize = m_iWritePos - m_iReadPos;
    return iSize;
}

int CPacket::MoveReadPos(int iSize)
{
    if (iSize <= 0) return 0;
    int canMove = m_iWritePos - m_iReadPos;
    if (iSize > canMove) iSize = canMove;
    m_iReadPos += iSize;
    m_iDataSize = m_iWritePos - m_iReadPos;
    _CompactIfEmpty();
    return iSize;
}

// ============================== 대입 연산자 ==============================

CPacket& CPacket::operator = (CPacket& src)
{
    if (this == &src) return *this;
    if (m_iBufferSize < src.m_iWritePos)
    {
        delete[] m_chpBuffer;
        m_iBufferSize = max(src.m_iBufferSize, src.m_iWritePos);
        m_chpBuffer = new char[m_iBufferSize];
    }
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
    _EnsureCapacity(iSrcSize);
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

CPacket& CPacket::operator << (unsigned char byValue) { PutData(reinterpret_cast<char*>(&byValue), sizeof(byValue)); return *this; }
CPacket& CPacket::operator << (char chValue) { PutData(reinterpret_cast<char*>(&chValue), sizeof(chValue)); return *this; }
CPacket& CPacket::operator << (short shValue) { PutData(reinterpret_cast<char*>(&shValue), sizeof(shValue)); return *this; }
CPacket& CPacket::operator << (unsigned short wValue) { PutData(reinterpret_cast<char*>(&wValue), sizeof(wValue)); return *this; }
CPacket& CPacket::operator << (int iValue) { PutData(reinterpret_cast<char*>(&iValue), sizeof(iValue)); return *this; }
CPacket& CPacket::operator << (long lValue) { PutData(reinterpret_cast<char*>(&lValue), sizeof(lValue)); return *this; }
CPacket& CPacket::operator << (float fValue) { PutData(reinterpret_cast<char*>(&fValue), sizeof(fValue)); return *this; }
CPacket& CPacket::operator << (__int64 iValue) { PutData(reinterpret_cast<char*>(&iValue), sizeof(iValue)); return *this; }
CPacket& CPacket::operator << (double dValue) { PutData(reinterpret_cast<char*>(&dValue), sizeof(dValue)); return *this; }

// ============================== >> Deserialize ==============================

CPacket& CPacket::operator >> (char& chValue) { GetData(reinterpret_cast<char*>(&chValue), sizeof(chValue)); return *this; }
CPacket& CPacket::operator >> (unsigned char& byValue) { GetData(reinterpret_cast<char*>(&byValue), sizeof(byValue)); return *this; }
CPacket& CPacket::operator >> (short& shValue) { GetData(reinterpret_cast<char*>(&shValue), sizeof(shValue)); return *this; }
CPacket& CPacket::operator >> (unsigned short& wValue) { GetData(reinterpret_cast<char*>(&wValue), sizeof(wValue)); return *this; }
CPacket& CPacket::operator >> (int& iValue) { GetData(reinterpret_cast<char*>(&iValue), sizeof(iValue)); return *this; }
CPacket& CPacket::operator >> (unsigned int& dwValue) { GetData(reinterpret_cast<char*>(&dwValue), sizeof(dwValue)); return *this; }
CPacket& CPacket::operator >> (float& fValue) { GetData(reinterpret_cast<char*>(&fValue), sizeof(fValue)); return *this; }
CPacket& CPacket::operator >> (__int64& iValue) { GetData(reinterpret_cast<char*>(&iValue), sizeof(iValue)); return *this; }
CPacket& CPacket::operator >> (double& dValue) { GetData(reinterpret_cast<char*>(&dValue), sizeof(dValue)); return *this; }
