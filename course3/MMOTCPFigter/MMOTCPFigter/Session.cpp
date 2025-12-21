#include "Session.h"

#include "Protocol.h"
#include "CRingBuffer.h"
#include "MessageQueue.h"
#include "OVERLAPPED_CONTEXT.h"

SOCKETINFO::SOCKETINFO()
{
	//InitializeCriticalSection(&session_cs);
	sock = INVALID_SOCKET;
	recvBuf = nullptr;
	sendBuf = nullptr;
	messageQueue = nullptr;
	sendOverlapped = new OVERLAPPED_CONTEXT(ESend);
	recvOverlapped = new OVERLAPPED_CONTEXT(ERecv);

	dwAction = 0;
	byDirection=dfPACKET_MOVE_DIR_RR;
	shX =(rand() % (dfRANGE_MOVE_RIGHT - dfRANGE_MOVE_LEFT + 1)) + dfRANGE_MOVE_LEFT;
	shY = (rand() % (dfRANGE_MOVE_BOTTOM - dfRANGE_MOVE_TOP + 1)) + dfRANGE_MOVE_TOP;
	chHP = 100;
}

SOCKETINFO::SOCKETINFO(int bufsize)
{
	//InitializeCriticalSection(&session_cs);
	sock = INVALID_SOCKET;
	recvBuf = new CRingBuffer(bufsize + 1);
	sendBuf = new CRingBuffer(bufsize + 1);
	messageQueue = new MessageQueue(bufsize);
	sendOverlapped = new OVERLAPPED_CONTEXT(ESend);
	recvOverlapped = new OVERLAPPED_CONTEXT(ERecv);

	dwAction = 0;
	byDirection = dfPACKET_MOVE_DIR_RR;
	shX = (rand() % (dfRANGE_MOVE_RIGHT - dfRANGE_MOVE_LEFT + 1)) + dfRANGE_MOVE_LEFT;
	shY = (rand() % (dfRANGE_MOVE_BOTTOM - dfRANGE_MOVE_TOP + 1)) + dfRANGE_MOVE_TOP;
	chHP = 100;
}

SOCKETINFO::~SOCKETINFO()
{
	//DeleteCriticalSection(&session_cs);

	delete recvBuf;
	recvBuf = nullptr;

	delete sendBuf;
	sendBuf = nullptr;

	delete messageQueue;
	messageQueue = nullptr;

	delete sendOverlapped;
	sendOverlapped = nullptr;

	delete recvOverlapped;
	recvOverlapped = nullptr;
}

void SOCKETINFO::GetSessionLock()
{
	//EnterCriticalSection(&session_cs);
}

void SOCKETINFO::UnLockSession()
{
	//LeaveCriticalSection(&session_cs);
}

void SOCKETINFO::DecreaseIOCount()
{
	//EnterCriticalSection(&session_cs);
	InterlockedDecrement((long*)&IOCount);
	//LeaveCriticalSection(&session_cs);
}