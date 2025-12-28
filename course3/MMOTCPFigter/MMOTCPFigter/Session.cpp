#include "Session.h"
#include "Protocol.h"


SOCKETINFO::SOCKETINFO()
{
	sock = INVALID_SOCKET;
	Arrayindex = -1;
	session_id = -1;
	recvBuf.ClearBuffer();
	sendBuf.ClearBuffer();
	pCharacter = nullptr;
}

SOCKETINFO::SOCKETINFO(int bufsize)
{
	sock = INVALID_SOCKET;

	pCharacter = nullptr;
}

SOCKETINFO::~SOCKETINFO()
{
	sock = INVALID_SOCKET;

	pCharacter = nullptr;
}

void SOCKETINFO::OnAccept() {
	// 세션이 풀에서 꺼내져 재사용될 때
	sock = INVALID_SOCKET;
	Arrayindex = -1;
	session_id = -1;
	sendBuf.ClearBuffer();
	recvBuf.ClearBuffer();
	pCharacter = nullptr;
	// 기타 세션 정보 초기화
}

void SOCKETINFO::OnRelease() {
	// 세션이 끊겨서 풀에 들어갈 때
	sock = INVALID_SOCKET;
	Arrayindex = -1;
	session_id = -1;
	sendBuf.ClearBuffer(); // 데이터만 초기화, 메모리 해제 안 함!
	recvBuf.ClearBuffer();
	pCharacter = nullptr;
}

