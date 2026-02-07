#include "Session.h"
#include "CRingBuffer.h"
#include  "CPacketRingBuffer.h"
#include "MessageQueue.h"
#include "OVERLAPPED_CONTEXT.h"

#define BUFSIZE (1024 * 16)



SOCKETINFO::SOCKETINFO()
{
	//InitializeCriticalSection(&session_cs);
	_sock = INVALID_SOCKET;
	_recvBuf = new CRingBuffer(BUFSIZE + 1);
	_sendBuf = new CPacketRingBuffer(BUFSIZE + 1);
	messageQueue = new MessageQueue(BUFSIZE);
	_sendOverlapped = new OVERLAPPED_CONTEXT(ESend);
	_recvOverlapped = new OVERLAPPED_CONTEXT(ERecv);
}

SOCKETINFO::~SOCKETINFO()
{
	//DeleteCriticalSection(&session_cs);

	delete _recvBuf;
	_recvBuf = nullptr;

	delete _sendBuf;
	_sendBuf = nullptr;

	delete messageQueue;
	messageQueue = nullptr;

	delete _sendOverlapped;
	_sendOverlapped = nullptr;

	delete _recvOverlapped;
	_recvOverlapped = nullptr;
}

void SOCKETINFO::Inintialize(SOCKET sock, SessionKey sessionKey)
{
	_Active = true;
	_sock = sock;
	_sessionKey = sessionKey;
	_IsSending = 0;
	_IOCount = 0;

	//OVERLAPPED의 맴버가 초기화 되지 않도록 해준다.
	//nullptr을 참조하는 상황이 나오지 않게하기위해 순서를 조절한다.
	//recvOverlapped->op = ERecv;
	ZeroMemory(_recvOverlapped, sizeof(OVERLAPPED));

	//sendOverlapped->op = ESend;
	ZeroMemory(_sendOverlapped, sizeof(OVERLAPPED));
	
	_recvBuf->ClearBuffer();
	_sendBuf->ClearBuffer();
}

void SOCKETINFO::DecreaseIOCount()
{
	//EnterCriticalSection(&session_cs);
	InterlockedDecrement((unsigned long*)&_IOCount);
	//LeaveCriticalSection(&session_cs);
}

//SOCKETINFO::SOCKETINFO()
//{
//	//InitializeCriticalSection(&session_cs);
//	_sock = INVALID_SOCKET;
//	recvBuf = nullptr;
//	sendBuf = nullptr;
//	messageQueue = nullptr;
//	sendOverlapped = new OVERLAPPED_CONTEXT(ESend);
//	recvOverlapped = new OVERLAPPED_CONTEXT(ERecv);
//}

//void SOCKETINFO::GetSessionLock()
//{
//	//EnterCriticalSection(&session_cs);
//}
//
//void SOCKETINFO::UnLockSession()
//{
//	//LeaveCriticalSection(&session_cs);
//}