#pragma once
/////////////////////////////////////////////////////////////////////
// www.gamecodi.com						이주행 master@gamecodi.com
//
//
/////////////////////////////////////////////////////////////////////
/*---------------------------------------------------------------

	Packet.

	네트워크 패킷용 클래스.
	간편하게 패킷에 순서대로 데이타를 In, Out 한다.

	- 사용법.

	CPacket cPacket;  or CMessage Message;

	넣기.
	clPacket << 40030;		or	clPacket << iValue;	(int 넣기)
	clPacket << 1.4;		or	clPacket << fValue;	(float 넣기)


	빼기.
	clPacket >> iValue;		(int 빼기)
	clPacket >> byValue;		(BYTE 빼기)
	clPacket >> fValue;		(float 빼기)

	CPacket Packet2;

	!.	삽입되는 데이타 FIFO 순서로 관리된다.
		환형 큐는 아니므로, 넣기(<<).빼기(>>) 를 혼합해서 사용하지 않도록 한다



	* 실제 패킷 프로시저에서의 처리

	BOOL	netPacketProc_CreateMyCharacter(CPacket *clpPacket)
	{
		DWORD dwSessionID;
		short shX, shY;
		char chHP;
		BYTE byDirection;

//		*clpPacket >> dwSessionID >> byDirection >> shX >> shY >> chHP;


		*clpPacket >> dwSessionID;
		*clpPacket >> byDirection;
		*clpPacket >> shX;
		*clpPacket >> shY;
		*clpPacket >> chHP;

		...
		...
	}


	* 실제 메시지(패킷) 생성부에서의 처리

	CPacket MoveStart;
	mpMoveStart(&MoveStart, dir, x, y);
	SendPacket(&MoveStart);


	void	mpMoveStart(CPacket *clpPacket, BYTE byDirection, short shX, short shY)
	{
		st_NETWORK_PACKET_HEADER	stPacketHeader;
		stPacketHeader.byCode = dfNETWORK_PACKET_CODE;
		stPacketHeader.bySize = 5;
		stPacketHeader.byType = dfPACKET_CS_MOVE_START;

		clpPacket->PutData((char *)&stPacketHeader, dfNETWORK_PACKET_HEADER_SIZE);

		*clpPacket << byDirection;
		*clpPacket << shX;
		*clpPacket << shY;

	}

----------------------------------------------------------------*/
#ifndef  __PACKET__
#define  __PACKET__

#include"MemoryPoolForLockFree.h"

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

	/*---------------------------------------------------------------
	Packet Enum.

	----------------------------------------------------------------*/
	enum en_PACKET
	{
		eBUFFER_DEFAULT = 1400		// 패킷의 기본 버퍼 사이즈.
	};




	//////////////////////////////////////////////////////////////////////////
	// 패킷 청소.
	//
	// Parameters: 없음.
	// Return: 없음.
	//////////////////////////////////////////////////////////////////////////
	void	Clear(void);


	//////////////////////////////////////////////////////////////////////////
	// 버퍼 사이즈 얻기.
	//
	// Parameters: 없음.
	// Return: (int)패킷 버퍼 사이즈 얻기.
	//////////////////////////////////////////////////////////////////////////
	int	 GetBufferSize(void) { return m_iBufferSize; }
	//////////////////////////////////////////////////////////////////////////
	// 현재 사용중인 사이즈 얻기.
	//
	// Parameters: 없음.
	// Return: (int)사용중인 데이타 사이즈.
	//////////////////////////////////////////////////////////////////////////
	int		GetDataSize(void) { return m_iDataSize; }



	//////////////////////////////////////////////////////////////////////////
	// 버퍼 포인터 얻기.
	//
	// Parameters: 없음.
	// Return: (char *)버퍼 포인터.
	//////////////////////////////////////////////////////////////////////////
	char* GetBufferPtr(void) { return m_chpBuffer; }

	//////////////////////////////////////////////////////////////////////////
	// 버퍼 Pos 이동. (음수이동은 안됨)
	// GetBufferPtr 함수를 이용하여 외부에서 강제로 버퍼 내용을 수정할 경우 사용. 
	//
	// Parameters: (int) 이동 사이즈.
	// Return: (int) 이동된 사이즈.
	//////////////////////////////////////////////////////////////////////////
	int		MoveWritePos(int iSize);
	int		MoveReadPos(int iSize);






	/* ============================================================================= */
	// 연산자 오버로딩
	/* ============================================================================= */
	CPacket& operator = (CPacket& clSrcPacket);

	//////////////////////////////////////////////////////////////////////////
	// 넣기.	각 변수 타입마다 모두 만듬.
	//////////////////////////////////////////////////////////////////////////
	CPacket& operator << (unsigned char byValue);
	CPacket& operator << (char chValue);

	CPacket& operator << (short shValue);
	CPacket& operator << (unsigned short wValue);

	CPacket& operator << (int iValue);
	CPacket& operator << (long lValue);
	CPacket& operator << (float fValue);

	CPacket& operator << (__int64 iValue);
	CPacket& operator << (double dValue);


	//////////////////////////////////////////////////////////////////////////
	// 빼기.	각 변수 타입마다 모두 만듬.
	//////////////////////////////////////////////////////////////////////////
	CPacket& operator >> (char& chValue);
	CPacket& operator >> (unsigned char& byValue);

	CPacket& operator >> (short& shValue);
	CPacket& operator >> (unsigned short& wValue);

	CPacket& operator >> (int& iValue);
	CPacket& operator >> (unsigned int& dwValue);
	CPacket& operator >> (float& fValue);

	CPacket& operator >> (__int64& iValue);
	CPacket& operator >> (double& dValue);




	//////////////////////////////////////////////////////////////////////////
	// 데이타 얻기.
	//
	// Parameters: (char *)Dest 포인터. (int)Size.
	// Return: (int)복사한 사이즈.
	//////////////////////////////////////////////////////////////////////////
	int		GetData(char* chpDest, int iSize);

	//////////////////////////////////////////////////////////////////////////
	// 데이타 삽입.
	//
	// Parameters: (char *)Src 포인터. (int)SrcSize.
	// Return: (int)복사한 사이즈.
	//////////////////////////////////////////////////////////////////////////
	int		PutData(char* chpSrc, int iSrcSize);

	inline static myMemorypool::CMemoryPool<CPacket> packetPool = myMemorypool::CMemoryPool<CPacket>(1024, true);

protected:
	//////////////////////////////////////////////////////////////////////////
	// 생성자, 파괴자.
	//
	// Return:
	//////////////////////////////////////////////////////////////////////////
	CPacket();
	CPacket(int iBufferSize);

	virtual	~CPacket();

	// 내부 유틸
	void    _EnsureCapacity(int requireBytes);
	void    _CompactIfEmpty(); // 다 읽었으면 포인터 리셋

	char* m_chpBuffer = nullptr;

	// 전체 버퍼 크기
	int     m_iBufferSize = 0;

	// 현재 버퍼에서 사용중인 사이즈 (write - read)
	int     m_iDataSize = 0;

	// 읽기/쓰기 위치
	int     m_iReadPos = 0;
	int     m_iWritePos = 0;

	long long mRefCount = 0;
};



#endif
