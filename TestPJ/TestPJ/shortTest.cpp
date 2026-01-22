#include "stdafx.h"
#include "CPacket.h"
#include <unordered_map>

WORD	MessageLen;
WCHAR Message[20] = {1,2,3};

int main() {
	CPacket pack;
	pack.PutData((char*)Message, sizeof(Message) / sizeof(WCHAR));
	MessageLen = sizeof(Message) / sizeof(WCHAR);
	pack << MessageLen;

	WORD	getMessageLen;
	WCHAR* getMessage;
	pack >> getMessageLen;
	pack.GetData((char*)getMessage, getMessageLen);

	printf("%s", getMessage);
}