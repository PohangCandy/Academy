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

