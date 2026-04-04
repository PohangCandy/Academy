#pragma once
//------------------------------------------------------------
// ChattingServer_MultiThread
//
// [핵심 변경] ContentsThread 제거, 워커 스레드가 직접 채팅 로직 처리
// - SRWLock으로 공유 자료구조(캐릭터맵, 섹터맵) 동기화
// - Exclusive Lock: 캐릭터 생성/삭제, 섹터 이동
// - Shared Lock: 캐릭터 조회, 채팅 브로드캐스트
// - 메모리 최적화: 세션당 ~8KB (기존 ~144KB)
//------------------------------------------------------------

#include "CLanServer.h"
#include "CMonitorClient.h"
#include "CpuUsage.h"
#include <Pdh.h>
#include <Psapi.h>
#include <unordered_map>
#include <vector>

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

	volatile long long _lastRecvTime;
};



class ChattingServer : public CLanServer
{
public:
	ChattingServer();
	~ChattingServer();

	Character* FindCharacter(SessionKey sessionkey);

	bool DeleteCharacter(SessionKey sessionkey);

	bool CreateCharacter(SessionKey sessionkey);

	//------------------------------------------------------------
	// [추가] 모니터링 서버 접속
	//------------------------------------------------------------
	bool ConnectMonitor(const char* monitorIP, int monitorPort, int serverNo);
	CMonitorClient* GetMonitorClient() { return &_monitorClient; }

private:

	//------------------------------------------------------------
	// [MultiThread] ContentsThread 제거
	// 워커 스레드에서 직접 채팅 로직 처리
	//------------------------------------------------------------

	// 타이머 스레드 (하트비트 체크 + 모니터링)
	static unsigned int __stdcall TimerThread(LPVOID arg);
	HANDLE hTimerThread = {};
	bool _bIsTimerThreadAlive = true;

	virtual bool OnConnectionRequest(std::string IP, int Port) override;

	virtual void	OnClientJoin(SOCKADDR_IN clientaddr, SessionKey s) override;

	virtual void 	OnClientLeave(SessionKey s) override;

	virtual void 	OnRecv(SessionKey s, CPacket* pPacket)  override;

	virtual void OnError(int errorcode, const char* msg) override;

	//------------------------------------------------------------
	// [MultiThread] 메시지 처리 함수 (워커 스레드에서 직접 호출)
	//------------------------------------------------------------
	void Handle_CS_CHAT_REQ_LOGIN(SessionKey sessionkey, CPacket* pPacket);
	void Handle_CS_CHAT_REQ_SECTOR_MOVE(SessionKey sessionkey, CPacket* pPacket);
	void Handle_CS_CHAT_REQ_MESSAGE(SessionKey sessionkey, CPacket* pPacket);
	void Handle_CS_CHAT_REQ_HEARTBEAT(SessionKey sessionkey, CPacket* pPacket);

	EServerMode _serverMode = None;

	//------------------------------------------------------------
	// [MultiThread] SRWLock으로 공유 자료구조 보호
	//------------------------------------------------------------
	SRWLOCK _characterLock;

	// 캐릭터맵, 섹터맵 (멤버 변수로 이동)
	std::unordered_map<uint64_t, Character*> _umapCharacter;
	std::unordered_map<uint64_t, Character*> _umapCharacterSector[50][50];

	//------------------------------------------------------------
	// [추가] 모니터링 클라이언트
	//------------------------------------------------------------
	CMonitorClient _monitorClient;

	//------------------------------------------------------------
	// CPU/메모리 수집용
	//------------------------------------------------------------
	CpuUsage _cpuUsage;

	//------------------------------------------------------------
	// [추가] CPU/메모리 수집용
	//------------------------------------------------------------
	PDH_HQUERY		_cpuQuery = NULL;
	PDH_HCOUNTER	_cpuCounter = NULL;

	//------------------------------------------------------------
	// [추가] TPS 카운터
	//------------------------------------------------------------
	alignas(64) long _updateCount = 0;		// 워커 스레드에서 처리한 메시지 수 (1초마다 리셋)

	//------------------------------------------------------------
	// [추가] 디스플레이용 최신 값 (TimerThread에서 갱신)
	//------------------------------------------------------------
	int _dispCpu = 0;
	int _dispMemMB = 0;
	int _dispSessionCount = 0;
	int _dispPlayerCount = 0;
	int _dispUpdateTPS = 0;
	int _dispPacketPoolUse = 0;
	int _dispAcceptTPS = 0;
	int _dispMonitorSendCount = 0;
	bool _dispMonitorConnected = false;
	volatile LONG _heartbeatTimeoutCount = 0;

	//------------------------------------------------------------
	// [추가] 스레드 활동 추적 (진단용)
	// 각 작업에 진입 중인 스레드 수를 카운팅
	// 서버 정지 시 어디서 스레드가 멈췄는지 확인 가능
	//------------------------------------------------------------
	alignas(64) volatile LONG _activeInJoin = 0;
	volatile LONG _activeInLeave = 0;
	volatile LONG _activeInLogin = 0;
	volatile LONG _activeInSectorMove = 0;
	volatile LONG _activeInChatMsg = 0;
	volatile LONG _activeInHeartbeat = 0;
	volatile LONG _activeInTimerLock = 0;
};
