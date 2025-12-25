#include "Session.h"
#include "Protocol.h"


SOCKETINFO::SOCKETINFO()
{
	sock = INVALID_SOCKET;

	pCharacter = nullptr;
}

SOCKETINFO::SOCKETINFO(int bufsize)
{
	sock = INVALID_SOCKET;

	pCharacter = nullptr;
}

