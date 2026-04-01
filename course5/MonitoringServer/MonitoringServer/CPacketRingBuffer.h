#pragma once
#include "stdafx.h"

class CPacket;

class CPacketRingBuffer {
public:
	CPacketRingBuffer(void);
	CPacketRingBuffer(int iBufferSize);
	~CPacketRingBuffer();

	int GetBufferSize(void);
	bool Enqueue(CPacket* cpacket);
	bool Dequeue(CPacket*& pPacket);
	void ClearBuffer(void);
	int MoveRear(int iSize);
	int MoveFront(int iSize);
	CPacket** GetFrontBufferPtr(void);
	CPacket** GetRearBufferPtr(void);
	CPacket** GetBufPtr(void);

	int GetUseSize(void);
	int GetFreeSize(void);
	int GetFront();

	void Lock() { EnterCriticalSection(&_csRingbuffer); }
	void UnLock() { LeaveCriticalSection(&_csRingbuffer); }

private:
	CRITICAL_SECTION _csRingbuffer;
	CPacket** _packetBuffer;
	int _capacity;
	int _front;
	int _rear;
	bool _IsFull;
};
