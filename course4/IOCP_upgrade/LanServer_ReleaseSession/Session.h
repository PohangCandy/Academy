//SOCKETINFO.h
#pragma once
#include "stdafx.h"

class MessageQueue;
class CRingBuffer;
class CPacketRingBuffer;
struct OVERLAPPED_CONTEXT;

//소켓 정보 저장을 위한 클래스
class SOCKETINFO
{
public:
	SOCKETINFO();
	SOCKETINFO(int bufsize);
	~SOCKETINFO();

	void Inintialize(SOCKET sock, long long sessionID);

	//void GetSessionLock();

	//void UnLockSession();

	void DecreaseIOCount();


	//CRITICAL_SECTION session_cs;
	SOCKET _sock;
	//지금은 그냥 객체 자체가 들어가있는데 포인터가 들어가는게 맞아보임.
	//안그러면 세션 객체 크기가 너무 커짐. 딱히 문제는 없어보이는데 문제가 있을까?
	CRingBuffer* recvBuf;
	CPacketRingBuffer* sendBuf;
	MessageQueue* messageQueue;
	long long session_id = 0;
	LONG IsSending = 0;
	int sendPacketNum = 0;
	long IOCount = 0;
	OVERLAPPED_CONTEXT* sendOverlapped;
	OVERLAPPED_CONTEXT* recvOverlapped;
	//OVERLAPPED_CONTEXT contentsOverlapped{ EContents };
};