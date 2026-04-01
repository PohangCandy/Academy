#pragma once
#include "CLanServer.h"
#include "CMonitorClient.h"
#include <Pdh.h>
#include <Psapi.h>
#include <map>
#include <list>
#include <unordered_map>

#pragma comment(lib, "Pdh.lib")

enum EServerMode {
	None,
	QA
};

class Character {
public:

	void OnReuse();

	INT64	_AccountNo;
	WCHAR	_ID[20];	// null 포함
	WCHAR	_Nickname[20];	// null 포함
	char	_Token[64];		// 세션토큰

	WORD	_SectorX = -1;
	WORD	_SectorY = -1;
	SessionKey _sessionkey;

	long long _lastRecvTime;
	//bool _bDie;
};



class ChattingServer : public CLanServer
{
public:
	ChattingServer();
	~ChattingServer();

	bool _bIsTimerThreadRuning() { return _bIsTimerThreadAlive; }

	Character* FindCharacter(SessionKey sessionkey);

	bool DeleteCharacter(SessionKey sessionkey);

	bool CreateCharacter(SessionKey sessionkey);

	//------------------------------------------------------------
	// [추가] 모니터링 서버 접속
	//------------------------------------------------------------
	bool ConnectMonitor(const char* monitorIP, int monitorPort, int serverNo);
	CMonitorClient* GetMonitorClient() { return &_monitorClient; }

private:

	//컨텐츠 스레드 함수
	static unsigned int __stdcall ContentsThread(LPVOID arg);

	//타이머 스레드 함수
	static unsigned int __stdcall TimerThread(LPVOID arg);

	HANDLE hContentCompletionPort = {};
	HANDLE hContentThread = {};
	HANDLE hTimerThread = {};

	bool _bIsTimerThreadAlive = true;

	virtual bool OnConnectionRequest(std::string IP, int Port) override;

	virtual void	OnClientJoin(SOCKADDR_IN clientaddr, SessionKey s) override;

	virtual void 	OnClientLeave(SessionKey s) override;

	virtual void 	OnRecv(SessionKey s, CPacket* pPacket)  override;

	virtual void OnError(int errorcode, const char* msg) override;

	EServerMode _serverMode = None;

	//------------------------------------------------------------
	// [추가] 모니터링 클라이언트
	//------------------------------------------------------------
	CMonitorClient _monitorClient;

	//------------------------------------------------------------
	// [추가] CPU/메모리 수집용
	//------------------------------------------------------------
	PDH_HQUERY		_cpuQuery = NULL;
	PDH_HCOUNTER	_cpuCounter = NULL;

	//------------------------------------------------------------
	// [추가] TPS / 메시지큐 카운터
	//------------------------------------------------------------
	alignas(64) long _updateCount = 0;		// ContentsThread에서 처리한 메시지 수 (1초마다 리셋)
	alignas(64) long _msgQueueSize = 0;		// 컨텐츠 IOCP 큐에 대기 중인 메시지 수
};
