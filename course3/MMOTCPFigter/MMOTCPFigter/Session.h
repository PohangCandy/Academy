//SOCKETINFO.h
#pragma once
#include "stdafx.h"
#include "CRingBuffer.h"

class c_CHARACTER;

//소켓 정보 저장을 위한 클래스
class SOCKETINFO
{
public:
	SOCKETINFO();
	SOCKETINFO(int bufsize);
	~SOCKETINFO();

	void OnAccept();
	void OnRelease();

	bool Active = true;
	int Arrayindex = -1;
	int session_id = -1;
	SOCKET sock;
	CRingBuffer recvBuf;
	CRingBuffer sendBuf;

	int dwLastRecvTime = 0;

	c_CHARACTER* pCharacter;
};