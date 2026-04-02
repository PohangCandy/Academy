#pragma once
//------------------------------------------------------------
// CLanClient - 내부 LAN 서버 접속용 IOCP 클라이언트
//
// - WORD Len 헤더 (암호화 없음)
// - 자체 워커 스레드 보유
// - 이벤트 함수(접속완료, 수신완료, 끊김) 가상 함수로 제공
//------------------------------------------------------------

#include "stdafx.h"
#include <atomic>
#include <string>

class CPacket;
class CRingBuffer;
class CPacketRingBuffer;
struct OVERLAPPED_CONTEXT;

class CLanClient
{
public:
	CLanClient();
	virtual ~CLanClient();

	bool Connect(const char* serverIP, int serverPort, int workerThreadCount = 1, bool bNagle = false);
	void Stop();
	bool Disconnect();
	bool SendPacket(CPacket* cp);

	bool IsConnected() const { return _bConnected; }

protected:
	virtual void OnEnterJoinServer() = 0;
	virtual void OnLeaveServer() = 0;
	virtual void OnRecv(CPacket* pPacket) = 0;
	virtual void OnError(int errorcode, const char* msg) = 0;

private:
	bool WsaRecvPost();
	bool CanSend();
	bool SendPost();
	void Release();

	static unsigned int __stdcall WorkerThread(LPVOID arg);

	HANDLE _hIOCP;
	SOCKET _sock;

	// 스레드 핸들 (종료 대기용)
	static const int MAX_WORKER_THREADS = 4;
	HANDLE _hWorkerThreads[MAX_WORKER_THREADS];
	int _workerThreadCount = 0;

	CRingBuffer* _recvBuf;
	CPacketRingBuffer* _sendBuf;
	OVERLAPPED_CONTEXT* _recvOverlapped;
	OVERLAPPED_CONTEXT* _sendOverlapped;

	volatile long _IsSending;
	int _sendPacketNum;

	// IOCount + Release Flag (bit31)
	volatile long _IOCount;
	bool _bConnected;
};
