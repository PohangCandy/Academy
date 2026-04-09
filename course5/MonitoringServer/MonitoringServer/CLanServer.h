#pragma once
//------------------------------------------------------------
// CLanServer - 내부 LAN 통신용 IOCP 서버
//
// ChattingServer_SingleThread의 CLanServer를 기반으로 하되:
// [변경1] 단순 WORD Len 헤더 사용 (암호화 없음, 내부 통신)
// [변경2] Accept를 별도 스레드로 분리 (Start()가 블로킹되지 않음)
// [변경3] 자체 cSessionMap 인스턴스 소유 (싱글턴 제거)
// [변경4] _CrtSetDbgFlag를 main으로 이동
//------------------------------------------------------------

#include "stdafx.h"
#include "SessionKey.h"
#include <atomic>

class CPacket;
class SOCKETINFO;
class cSessionMap;
struct LanPacketHeader;

class CLanServer
{
public:
	CLanServer();
	virtual ~CLanServer();

	bool Start(int port, int maxSession);
	void Stop();
	int GetSessionCount();

	bool Disconnect(SessionKey sessionId);
	bool SendPacket(SessionKey sessionId, CPacket* cp);

protected:

	// 가상 콜백 함수
	virtual bool OnConnectionRequest(std::string IP, int Port) = 0;
	virtual void OnClientJoin(SOCKADDR_IN Client, SessionKey s) = 0;
	virtual void OnClientLeave(SessionKey s) = 0;
	virtual void OnRecv(SessionKey s, CPacket* pPacket) = 0;
	virtual void OnError(int errorcode, const char* msg) = 0;

private:

	bool WsaRecvSession(SOCKETINFO* ptr);
	bool SendPost(SOCKETINFO* ptr);
	bool CanSend(SOCKETINFO* ptr);

	static unsigned int __stdcall AcceptThread(LPVOID arg);
	static unsigned int __stdcall WorkerThread(LPVOID arg);
	static unsigned int __stdcall MonitorThread(LPVOID arg);

	HANDLE _hWorkerThreadIOCP;
	SOCKET _listenSock;
	cSessionMap* _pSessionMap;
	int _maxSession = 0;
	int _workerThreadCount = 0;

	// 스레드 핸들 (종료 대기용)
	static const int MAX_WORKER_THREADS = 64;
	HANDLE _hWorkerThreads[MAX_WORKER_THREADS];
	HANDLE _hAcceptThread = NULL;
	HANDLE _hMonitorThread = NULL;
	volatile bool _isRunning = false;

	std::atomic<int> _sessionCount{ 0 };
	std::atomic<int> _acceptCount{ 0 };
	std::atomic<int> _acceptTPS{ 0 };
	// 누적 카운터 (공격 패킷)
	alignas(64) volatile long _invalidPacketLenCount = 0;
	int _recvMessageTPS = 0;
	int _sendMessageTPS = 0;

public:
	int getAcceptTPS() { return _acceptTPS; }
	int getRecvMessageTPS() { return _recvMessageTPS; }
	int getSendMessageTPS() { return _sendMessageTPS; }
	long getInvalidPacketLenCount() { return _invalidPacketLenCount; }
};
