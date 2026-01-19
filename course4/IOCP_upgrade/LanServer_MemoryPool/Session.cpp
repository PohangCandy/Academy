#include "Session.h"

#include "CRingBuffer.h"
#include "MessageQueue.h"
#include "OVERLAPPED_CONTEXT.h"

SOCKETINFO::SOCKETINFO()
{
	//InitializeCriticalSection(&session_cs);
	_sock = INVALID_SOCKET;
	recvBuf = nullptr;
	sendBuf = nullptr;
	messageQueue = nullptr;
	sendOverlapped = new OVERLAPPED_CONTEXT(ESend);
	recvOverlapped = new OVERLAPPED_CONTEXT(ERecv);
}

SOCKETINFO::SOCKETINFO(int bufsize)
{
	//InitializeCriticalSection(&session_cs);
	_sock = INVALID_SOCKET;
	recvBuf = new CRingBuffer(bufsize + 1);
	sendBuf = new CRingBuffer(bufsize + 1);
	messageQueue = new MessageQueue(bufsize);
	sendOverlapped = new OVERLAPPED_CONTEXT(ESend);
	recvOverlapped = new OVERLAPPED_CONTEXT(ERecv);
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

void SOCKETINFO::Inintialize(SOCKET sock, long long sessionID)
{
	_sock = sock;
	session_id = sessionID;
	IsSending = 0;
	IOCount = 0;

	//OVERLAPPED의 맴버가 초기화 되지 않도록 해준다.
	//nullptr을 참조하는 상황이 나오지 않게하기위해 순서를 조절한다.
	recvOverlapped->op = ERecv;
	ZeroMemory(&recvOverlapped, sizeof(OVERLAPPED));

	sendOverlapped->op = ESend;
	ZeroMemory(&sendOverlapped, sizeof(OVERLAPPED));
	
	recvBuf->ClearBuffer();
	sendBuf->ClearBuffer();
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