#pragma once
//------------------------------------------------------------
// CNetServer - 외부 클라이언트 접속용 IOCP 서버
//
// 5바이트 암호화 헤더 (Code, Len, RandKey, CheckSum) 사용.
// LoginServer 가 게임 클라이언트의 로그인 요청을 받기 위한 프레임워크.
//
// ChattingServer_MultiThread / MonitoringServer 의 동일 클래스에 적용된
// 보안/안정화 패치를 그대로 상속:
//  - Len==0/Len>MAX 검증 -> Disconnect + counter
//  - RecvBuf overflow -> Disconnect (기존 __debugbreak 제거)
//  - 정상 종료성 WSA 에러 필터링
//  - Inintialize() 의 IOCount=1 (AcceptThread 소유권)
//  - SessionKey 재활용 ABA 가드
//  - 공격 패킷 누적 카운터
//------------------------------------------------------------

#include "stdafx.h"
#include "SessionKey.h"
#include <atomic>

class CPacket;
class SOCKETINFO;
class cSessionMap;

class CNetServer
{
public:
	CNetServer();
	virtual ~CNetServer();

	// workerThreadCount=0 이면 자동(논리코어 수). >0 이면 그 값 사용.
	bool Start(int port, int maxSession, int workerThreadCount = 0);
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
protected:
	int _maxSession = 0;
private:
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
	std::atomic<long long> _totalAcceptCount{ 0 };
	alignas(64) volatile long _sendBufferFullCount = 0;
	// 누적 카운터 (공격 패킷)
	alignas(64) volatile long _invalidPacketCodeCount = 0;
	alignas(64) volatile long _invalidPacketLenCount = 0;
	alignas(64) volatile long _decodeForNetFailCount = 0;
	int _recvMessageTPS = 0;
	int _sendMessageTPS = 0;

public:
	int getAcceptTPS() { return _acceptTPS; }
	long long getTotalAcceptCount() { return _totalAcceptCount.load(std::memory_order_relaxed); }
	long getSendBufferFullCount() { return _sendBufferFullCount; }
	long getInvalidPacketCodeCount() { return _invalidPacketCodeCount; }
	long getInvalidPacketLenCount() { return _invalidPacketLenCount; }
	long getDecodeForNetFailCount() { return _decodeForNetFailCount; }
	int getRecvMessageTPS() { return _recvMessageTPS; }
	int getSendMessageTPS() { return _sendMessageTPS; }
};
