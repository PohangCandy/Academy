#pragma once
#include "stdafx.h"
#include "SessionKey.h"

class CRingBuffer;
class CPacketRingBuffer;
struct OVERLAPPED_CONTEXT;

class SOCKETINFO
{
public:
	SOCKETINFO();
	~SOCKETINFO();

	void Inintialize(SOCKET sock, SessionKey sessionKey);

	SOCKET _sock;

	std::string _IP = {};
	int _PORT = 0;

	bool _Active = false;

	CRingBuffer* _recvBuf;
	CPacketRingBuffer* _sendBuf;

	SessionKey _sessionKey = { 0 };

	LONG _IsSending = 0;
	int _sendPacketNum = 0;

	// Release Flag[1], IOCount[31] Bit
	alignas(32) unsigned long _IOCount = 0;
	OVERLAPPED_CONTEXT* _sendOverlapped;
	OVERLAPPED_CONTEXT* _recvOverlapped;
};
