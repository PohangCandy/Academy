#include "Session.h"
#include "CRingBuffer.h"
#include "CPacketRingBuffer.h"
#include "OVERLAPPED_CONTEXT.h"

#define BUFSIZE (1024 * 16)

SOCKETINFO::SOCKETINFO()
{
	_sock = INVALID_SOCKET;
	_recvBuf = new CRingBuffer(BUFSIZE + 1);
	_sendBuf = new CPacketRingBuffer(BUFSIZE + 1);
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
	_IOCount = 0;

	ZeroMemory(_recvOverlapped, sizeof(OVERLAPPED));
	ZeroMemory(_sendOverlapped, sizeof(OVERLAPPED));

	_recvBuf->ClearBuffer();
	if (_sendBuf->GetUseSize() > 0) {
		__debugbreak();
	}
	_sendBuf->ClearBuffer();
}
