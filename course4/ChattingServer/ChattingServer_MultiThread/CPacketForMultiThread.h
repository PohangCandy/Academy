#pragma once
#ifndef  __PACKET__
#define  __PACKET__

#include "MemoryPoolForLockFree.h"

struct PacketHeader;
struct LanPacketHeader;

class CPacket
{
	friend class myMemorypool::CMemoryPool<CPacket>;

public:

	static CPacket* Alloc()
	{
		CPacket* allocPacket = packetPool.Alloc();
		allocPacket->Clear();
		return allocPacket;
	}

	//--------------------------------------------------------------
	// NET 인코딩 (WAN용 암호화 헤더)
	// packetCode, packetKey를 인자로 받아 다양한 서버에서 사용 가능
	//--------------------------------------------------------------
	bool IsEncoded = false;

	void EncodeForNet(unsigned char packetCode, unsigned char packetKey);

	//--------------------------------------------------------------
	// LAN 인코딩 (내부 LAN용 단순 WORD Len 헤더)
	//--------------------------------------------------------------
	void EncodeForLan();

	//-----------------------------------------
	// NET 디코딩
	//-----------------------------------------
	bool DecodeForNet(PacketHeader* pHeader, unsigned char packetKey);

	void AddRef()
	{
		InterlockedIncrement64(&mRefCount);
	}

	void SubRef()
	{
		if (InterlockedDecrement64(&mRefCount) == 0)
		{
			packetPool.Free(this);
		}
	}

	enum en_PACKET
	{
		eBUFFER_DEFAULT = 1400
	};

	void	Clear(void);

	int	 GetBufferSize(void) { return m_iBufferSize; }
	int	 GetDataSize(void) { return m_iDataSize; }

	char* GetBufferPtr(void) { return m_chpBuffer; }

	int		MoveWritePos(int iSize);
	int		MoveReadPos(int iSize);

	CPacket& operator = (CPacket& clSrcPacket);

	CPacket& operator << (unsigned char byValue);
	CPacket& operator << (char chValue);
	CPacket& operator << (short shValue);
	CPacket& operator << (unsigned short wValue);
	CPacket& operator << (int iValue);
	CPacket& operator << (long lValue);
	CPacket& operator << (float fValue);
	CPacket& operator << (__int64 iValue);
	CPacket& operator << (double dValue);

	CPacket& operator >> (char& chValue);
	CPacket& operator >> (unsigned char& byValue);
	CPacket& operator >> (short& shValue);
	CPacket& operator >> (unsigned short& wValue);
	CPacket& operator >> (int& iValue);
	CPacket& operator >> (unsigned int& dwValue);
	CPacket& operator >> (float& fValue);
	CPacket& operator >> (__int64& iValue);
	CPacket& operator >> (double& dValue);

	int		GetData(char* chpDest, int iSize);
	int		PutData(char* chpSrc, int iSrcSize);

	inline static myMemorypool::CMemoryPool<CPacket> packetPool = myMemorypool::CMemoryPool<CPacket>(1024, true);

	// 메시지 헤더 크기 (LAN=2, NET=5)
	int _MsgheaderSize = -1;

protected:
	CPacket();
	CPacket(int iBufferSize);
	virtual	~CPacket();

	void    _EnsureCapacity(int requireBytes);
	void    _CompactIfEmpty();

	char* m_chpBuffer = nullptr;

	int     m_iBufferSize = 0;
	int     m_iDataSize = 0;
	int     m_iReadPos = 0;
	int     m_iWritePos = 0;

	long long mRefCount = 0;
};

#endif
