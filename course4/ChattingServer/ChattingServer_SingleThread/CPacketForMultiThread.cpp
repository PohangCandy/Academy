#include "CPacketForMultiThread.h"
#include "CommonProtocol.h"

// ============================== 내부 유틸 ==============================

void CPacket::_EnsureCapacity(int requireBytes)
{
    // requireBytes는 추가로 "더 써야 하는" 용량.
    // 필요 총량 = m_iWritePos + requireBytes
    if (m_iWritePos + requireBytes <= m_iBufferSize) return;

    int newSize = max(m_iBufferSize, 1);
    while (newSize < m_iWritePos + requireBytes)
        newSize <<= 1; // 2배씩 확장

    char* newBuf = new (std::nothrow) char[newSize];
    if (!newBuf)
    {
        printf("[CPacket] 버퍼 확장에 실패\n");
        return;
    }

    // 기존 데이터 복사
    if (m_chpBuffer && m_iWritePos > 0)
        std::memcpy(newBuf, m_chpBuffer, m_iWritePos);

    delete[] m_chpBuffer;
    m_chpBuffer = newBuf;
    m_iBufferSize = newSize;
}

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

void CPacket::Encode()
{
    if (IsEncoded) return;

    unsigned char checksum = 0;

    if (_MsgheaderSize == -1)
    {
        printf("[CPacket/Encode] 메시지 헤더 크기가 없는데 이거 맞아?\n");
    }

    unsigned char randkey = rand() % 100;

    unsigned char paraP = 0;
    unsigned char encodeP = 0;


    int payLoadSize = m_iDataSize - _MsgheaderSize;

    for (int i = 0; i < payLoadSize; i++)
    {
        char* pPacketChar = &m_chpBuffer[_MsgheaderSize + i];
        checksum += *pPacketChar % 256;
    }
    m_chpBuffer[4] = checksum % 256;

    for (int i = 0; i < payLoadSize + sizeof(checksum); i++)
    {
        char* pPacketChar = &m_chpBuffer[_MsgheaderSize + i - sizeof(checksum)];
        paraP = *pPacketChar ^ (paraP + randkey + (i + 1));

        *pPacketChar = paraP ^ (encodeP + dfPACKET_KEY + (i + 1));
        encodeP = *pPacketChar;
    }

    //메시지 헤더 세팅
    memset(m_chpBuffer, 0, 4);
    m_chpBuffer[0] = dfPACKET_CODE;
    m_chpBuffer[1] = (short)payLoadSize;
    m_chpBuffer[3] = randkey;



    IsEncoded = true;
}

bool CPacket::Decode(PacketHeader* pHeader)
{
    //패킷에 담을 것이므로 pHeader의 체크섬 인코딩은 따로 진행한 후 맴버에 담고
    //패킷에 payload 데이터만 담아서 디코딩 시키는 걸로 진행 
    unsigned char* payload = (unsigned char*)m_chpBuffer +m_iReadPos;
    unsigned int checksum = 0;
    unsigned char beforeparaP = 0;
    unsigned char afterparaP = 0;
    unsigned char encodeP = 0;


    int payLoadSize = pHeader->Len;
    unsigned char* pPacketChar = &pHeader->CheckSum;

    afterparaP = *pPacketChar ^ (encodeP + dfPACKET_KEY + 1);
    encodeP = *pPacketChar;

    *pPacketChar = afterparaP ^ (beforeparaP + pHeader->RandKey + 1);
    beforeparaP = afterparaP;

    for (int i = 0; i < payLoadSize; i++)
    {
        pPacketChar = &payload[i];

        afterparaP = *pPacketChar ^ (encodeP + dfPACKET_KEY + (i + 2));
        encodeP = *pPacketChar;

        *pPacketChar = afterparaP ^ (beforeparaP + pHeader->RandKey + (i + 2));
        beforeparaP = afterparaP;

        checksum += *pPacketChar % 256;
        checksum %= 256;
    }



    //복호화가 제대로 이루어졌는지 확인
    if (pHeader->CheckSum != checksum)
    {
        printf("[Decode] checksum이 일치하지 않음. 복호화가 제대로 이루어지지 않음.\n");
        return false;
    }
    return true;
}

// ============================== 생성/소멸/관리 ==============================

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
    // 버퍼 내용은 굳이 지우지 않음(성능). 민감하면 memset.
}

// ============================== Move Pos ==============================

int CPacket::MoveWritePos(int iSize)
{
    if (iSize <= 0) return 0;
    _EnsureCapacity(iSize);
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
        delete[] m_chpBuffer;
        m_iBufferSize = max(src.m_iBufferSize, src.m_iWritePos);
        m_chpBuffer = new char[m_iBufferSize];
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
