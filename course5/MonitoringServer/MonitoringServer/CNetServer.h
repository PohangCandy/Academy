#pragma once
//------------------------------------------------------------
// CNetServer - 외부 WAN 통신용 IOCP 서버
//
// CLanServer와 동일한 구조이나:
// - 암호화된 5바이트 헤더 사용 (Code+Len+RandKey+CheckSum)
// - PACKET_CODE=109, PACKET_KEY=30 (모니터링 클라이언트 cnf 기준)
//------------------------------------------------------------

#include "stdafx.h"
#include "SessionKey.h"
#include <atomic>

class CPacket;
class SOCKETINFO;
class cSessionMap;
struct PacketHeader;

class CNetServer
{
public:
	CNetServer();
	virtual ~CNetServer();

	bool Start(int port, int maxSession);
	void Stop();
	int GetSessionCount();

	bool Disconnect(SessionKey sessionId);
	bool SendPacket(SessionKey sessionId, CPacket* cp);

protected:

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
	int _recvMessageTPS = 0;
	int _sendMessageTPS = 0;

public:
	int getAcceptTPS() { return _acceptTPS; }
	int getRecvMessageTPS() { return _recvMessageTPS; }
	int getSendMessageTPS() { return _sendMessageTPS; }
};
