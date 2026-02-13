//SOCKETINFO.h
#pragma once
#include "stdafx.h"
#include "SessionKey.h"

class MessageQueue;
class CRingBuffer;
class CPacketRingBuffer;
struct OVERLAPPED_CONTEXT;

//소켓 정보 저장을 위한 클래스
class SOCKETINFO
{
public:
	SOCKETINFO();
	~SOCKETINFO();

	void Inintialize(SOCKET sock, SessionKey sessionKey);

	SOCKET _sock;

	//-----------------------------
	// 세션에 맴버로 IP와 포트를 두는 게 디버깅에서 훨씬 더 편하여 맴버로 추가함.
	// ex) 세션을 다루는 함수에서 세션의 포르를 찍어보고 wireshark로 패킷을 관찰하는게 가장 큼.
	// 이게 없으면 계속 LanServer의 ClientsockAddr 인자를 함수로 넘겨줘서 코드가 길어짐.
	// 함수를 만드는 쪽이나 읽는 쪽이나 코드는 줄일수록 좋다고 생각함.
	//-----------------------------
	std::string _IP = {};
	int _PORT = 0;

	bool _Active = false;

	CRingBuffer* _recvBuf;
	CPacketRingBuffer* _sendBuf;
	//MessageQueue* messageQueue;

	//index[20], key[44] Bit
	SessionKey _sessionKey = {0};

	LONG _IsSending = 0;
	int _sendPacketNum = 0;

	//Release Flag[1], IOCount[31] Bit
	alignas(32) unsigned long _IOCount = 0;
	OVERLAPPED_CONTEXT* _sendOverlapped;
	OVERLAPPED_CONTEXT* _recvOverlapped;
	//OVERLAPPED_CONTEXT contentsOverlapped{ EContents };
};