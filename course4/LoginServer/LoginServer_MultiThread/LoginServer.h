#pragma once
//------------------------------------------------------------
// LoginServer (Multi-Thread)
//
// 구조:
//   [CNetServer]  -- 게임 클라 접속 (5바이트 암호화)
//        |
//        v OnRecv (IOCP 워커)
//        |  파싱 + DB 작업 큐 enqueue (DB 호출 금지)
//        v
//   [CDBJobQueue] -- DB 워커들이 pop
//        |
//        v
//   [CDBWorker thread × N]
//        |  mysql_query (blocking)
//        v
//   [CNetServer::SendPacket(sessionKey, resPacket)]  -- 응답
//
//   [CMonitorClient] -- MonitoringServer LAN 20000 으로 1초마다 지표 송신
//   [TimerThread]    -- 1초마다 콘솔 출력 + monitor 송신
//------------------------------------------------------------

#include "CNetServer.h"
#include "CDBJobQueue.h"
#include "CMonitorClient.h"
#include <vector>

// MySQL forward
struct st_mysql;
typedef st_mysql MYSQL;

class LoginServer : public CNetServer
{
public:
	LoginServer();
	~LoginServer();

	// ServerConfig.ini 의 값들 받아서 시작
	bool StartServer(int port, int maxSession,
	                 int netWorkerCount, int dbWorkerCount,
	                 const char* dbHost, int dbPort,
	                 const char* dbUser, const char* dbPasswd,
	                 const char* dbSchema,
	                 const char* monitorIP, int monitorPort, int serverNo);

	void StopServer();

	// 콘솔용 통계
	long long GetTotalLoginOK()    { return _totalLoginOK; }
	long long GetTotalLoginFail()  { return _totalLoginFail; }
	int       GetDBQueueSize()     { return _dbJobQueue.GetSize(); }
	int       GetAuthTPS()         { return _authTPS; }

private:
	//-- CNetServer 콜백 ----------------------------------------
	bool OnConnectionRequest(std::string IP, int Port) override;
	void OnClientJoin(SOCKADDR_IN clientaddr, SessionKey s) override;
	void OnClientLeave(SessionKey s) override;
	void OnRecv(SessionKey s, CPacket* pPacket) override;
	void OnError(int errorcode, const char* msg) override;

	//-- 패킷 핸들러 (IOCP 워커 컨텍스트, DB 호출 금지) -------------
	void Handle_CS_LOGIN_REQ_LOGIN(SessionKey s, CPacket* pPacket);

	//-- DB 워커 스레드 -----------------------------------------
	static unsigned int __stdcall DBWorkerThread(LPVOID arg);
	void DBWorkerLoop(MYSQL* conn);

	void Process_DBJob_Login(MYSQL* conn, const DBJob& job);

	//-- 타이머 / 통계 ------------------------------------------
	static unsigned int __stdcall TimerThread(LPVOID arg);

	//-- 멤버 ----------------------------------------------------
	CDBJobQueue   _dbJobQueue;
	std::vector<HANDLE> _hDBWorkers;
	int           _dbWorkerCount = 0;

	// DB 접속 정보 (워커가 자기 connection 만들 때 사용)
	std::string   _dbHost;
	int           _dbPort = 3306;
	std::string   _dbUser;
	std::string   _dbPasswd;
	std::string   _dbSchema;

	// 모니터링
	CMonitorClient _monitorClient;
	int            _serverNo = 0;
	HANDLE         _hTimerThread = NULL;
	volatile bool  _bTimerAlive = false;

	// 통계
	alignas(64) volatile long long _totalLoginOK = 0;
	alignas(64) volatile long long _totalLoginFail = 0;
	alignas(64) volatile long      _authCount = 0;
	int _authTPS = 0;
	int _dispDBQueueSize = 0;
};
