#include "Session.h"
#include "CRingBuffer.h"
#include "CPacketRingBuffer.h"
#include "OVERLAPPED_CONTEXT.h"

//------------------------------------------------------------
// [MultiThread] 메모리 최적화
// RecvBuf: 16KB -> 4KB (채팅 패킷은 최대 500바이트)
// SendBuf: 16385 -> 512 (동시 대기 패킷 수 제한)
// 세션당 ~8KB, 15,000세션 = ~120MB
//------------------------------------------------------------
#define RECV_BUFSIZE (1024 * 4)
#define SEND_BUFSIZE (512)

SOCKETINFO::SOCKETINFO()
{
	_sock = INVALID_SOCKET;
	_recvBuf = new CRingBuffer(RECV_BUFSIZE + 1);
	_sendBuf = new CPacketRingBuffer(SEND_BUFSIZE + 1);
	_sendOverlapped = new OVERLAPPED_CONTEXT(ESend);
	_recvOverlapped = new OVERLAPPED_CONTEXT(ERecv);
}

SOCKETINFO::~SOCKETINFO()
{
	delete _recvBuf;
	_recvBuf = nullptr;

	delete _sendBuf;
	_sendBuf = nullptr;

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
	_IOCount = 1;	// AcceptThread 소유권 (초기화 완료까지 세션 해제 방지)

	ZeroMemory(_recvOverlapped, sizeof(OVERLAPPED));
	ZeroMemory(_sendOverlapped, sizeof(OVERLAPPED));

	_recvBuf->ClearBuffer();
	if (_sendBuf->GetUseSize() > 0) {
		__debugbreak();
	}
	_sendBuf->ClearBuffer();
}
