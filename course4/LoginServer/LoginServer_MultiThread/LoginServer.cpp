#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "LoginServer.h"
#include "CPacketForMultiThread.h"
#include "CommonProtocol.h"
#include "CSystemLog.h"
#include "MemoryPoolForLockFree.h"
#include "C:\Program Files\MySQL\MySQL Server 8.0\include\mysql.h"
#include <process.h>
#include <ctime>

#pragma comment(lib, "libmysql.lib")

//------------------------------------------------------------
// 모니터링 데이터 타입은 CommonProtocol.h 의
// en_PACKET_SS_MONITOR_DATA_UPDATE 에 이미 정의되어 있으므로 그대로 사용.
//   dfMONITOR_DATA_TYPE_LOGIN_SESSION     (= 1)
//   dfMONITOR_DATA_TYPE_LOGIN_AUTH_TPS    (= 2)
//   dfMONITOR_DATA_TYPE_LOGIN_PACKET_POOL (= 3)
//   dfMONITOR_DATA_TYPE_LOGIN_SERVER_ON   (= 4)
//------------------------------------------------------------

LoginServer::LoginServer()
{
}

LoginServer::~LoginServer()
{
	StopServer();
}

bool LoginServer::StartServer(int port, int maxSession,
                              int netWorkerCount, int dbWorkerCount,
                              const char* dbHost, int dbPort,
                              const char* dbUser, const char* dbPasswd,
                              const char* dbSchema,
                              const char* monitorIP, int monitorPort, int serverNo)
{
	_dbHost   = dbHost;
	_dbPort   = dbPort;
	_dbUser   = dbUser;
	_dbPasswd = dbPasswd;
	_dbSchema = dbSchema;
	_serverNo = serverNo;

	//----------------------------------------------------------
	// 1. 서버 시작 시 status 테이블 초기화 (1회용 connection)
	//----------------------------------------------------------
	{
		MYSQL initConn;
		mysql_init(&initConn);
		if (!mysql_real_connect(&initConn, _dbHost.c_str(), _dbUser.c_str(),
		                        _dbPasswd.c_str(), _dbSchema.c_str(), _dbPort, NULL, 0))
		{
			LOG(L"LoginServer", CSystemLog::LEVEL_ERROR,
			    L"DB init connect failed: %S", mysql_error(&initConn));
			return false;
		}
		if (mysql_query(&initConn, "UPDATE status SET status = 0") != 0)
		{
			LOG(L"LoginServer", CSystemLog::LEVEL_ERROR,
			    L"status init query failed: %S", mysql_error(&initConn));
			mysql_close(&initConn);
			return false;
		}
		mysql_close(&initConn);
		LOG(L"LoginServer", CSystemLog::LEVEL_SYSTEM, L"DB status table initialized");
	}

	//----------------------------------------------------------
	// 2. DB 워커 스레드 풀 시작 (각자 connection)
	//----------------------------------------------------------
	if (dbWorkerCount <= 0) dbWorkerCount = 1;
	_dbWorkerCount = dbWorkerCount;

	for (int i = 0; i < _dbWorkerCount; i++)
	{
		unsigned int tid;
		HANDLE h = (HANDLE)_beginthreadex(NULL, 0, DBWorkerThread, this, 0, &tid);
		if (h == NULL)
		{
			LOG(L"LoginServer", CSystemLog::LEVEL_ERROR,
			    L"DB worker thread create failed (idx:%d)", i);
			return false;
		}
		_hDBWorkers.push_back(h);
	}
	LOG(L"LoginServer", CSystemLog::LEVEL_SYSTEM,
	    L"DB workers started: count=%d", _dbWorkerCount);

	//----------------------------------------------------------
	// 3. CNetServer 시작 (게임 클라 받는 IOCP 서버)
	//----------------------------------------------------------
	if (!CNetServer::Start(port, maxSession, netWorkerCount))
	{
		LOG(L"LoginServer", CSystemLog::LEVEL_ERROR,
		    L"CNetServer::Start failed (port:%d)", port);
		return false;
	}
	LOG(L"LoginServer", CSystemLog::LEVEL_SYSTEM,
	    L"NetServer started: port=%d, maxSession=%d, netWorkers=%d",
	    port, maxSession, netWorkerCount);

	//----------------------------------------------------------
	// 4. MonitorClient 접속 (best-effort. 실패해도 서버는 진행)
	//----------------------------------------------------------
	if (!_monitorClient.ConnectToMonitor(monitorIP, monitorPort, serverNo))
	{
		LOG(L"LoginServer", CSystemLog::LEVEL_ERROR,
		    L"MonitorClient connect failed (%S:%d) - continue without monitor",
		    monitorIP, monitorPort);
	}
	else
	{
		LOG(L"LoginServer", CSystemLog::LEVEL_SYSTEM,
		    L"MonitorClient connected (%S:%d serverNo=%d)",
		    monitorIP, monitorPort, serverNo);
	}

	//----------------------------------------------------------
	// 5. 타이머 스레드 (콘솔 출력 + monitor 송신)
	//----------------------------------------------------------
	_bTimerAlive = true;
	unsigned int tid;
	_hTimerThread = (HANDLE)_beginthreadex(NULL, 0, TimerThread, this, 0, &tid);
	if (_hTimerThread == NULL)
	{
		LOG(L"LoginServer", CSystemLog::LEVEL_ERROR, L"TimerThread create failed");
		return false;
	}

	return true;
}

void LoginServer::StopServer()
{
	// 1. NetServer 먼저 종료 (신규 접속 차단 + 기존 세션 정리 + 워커 스레드 join)
	CNetServer::Stop();

	// 2. DB 워커들에게 종료 신호 -> 처리 중인 작업은 마저 끝낸 후 종료
	_dbJobQueue.Stop();
	if (!_hDBWorkers.empty())
	{
		WaitForMultipleObjects((DWORD)_hDBWorkers.size(),
		                       _hDBWorkers.data(), TRUE, INFINITE);
		for (HANDLE h : _hDBWorkers) CloseHandle(h);
		_hDBWorkers.clear();
	}

	// 3. 타이머 스레드 종료
	_bTimerAlive = false;
	if (_hTimerThread != NULL)
	{
		WaitForSingleObject(_hTimerThread, INFINITE);
		CloseHandle(_hTimerThread);
		_hTimerThread = NULL;
	}

	// 4. 모니터 클라 정리
	_monitorClient.Stop();
}

//============================================================
// CNetServer 콜백 (모두 IOCP 워커 컨텍스트)
//============================================================

bool LoginServer::OnConnectionRequest(std::string IP, int Port)
{
	// 모든 접속 허용 (필요시 IP 블랙리스트 추가)
	return true;
}

void LoginServer::OnClientJoin(SOCKADDR_IN clientaddr, SessionKey s)
{
}

void LoginServer::OnClientLeave(SessionKey s)
{
}

void LoginServer::OnRecv(SessionKey s, CPacket* pPacket)
{
	// 핸들러 진입 즉시 Type 만 빼고 분기
	WORD type;

	if (pPacket->GetDataSize() < (int)sizeof(WORD))
	{
		// 비정상 패킷 - 헤더만 와있는 케이스. Disconnect
		Disconnect(s);
		return;
	}

	*pPacket >> type;

	switch (type)
	{
	case en_PACKET_CS_LOGIN_REQ_LOGIN:
		Handle_CS_LOGIN_REQ_LOGIN(s, pPacket);
		break;

	default:
		// 알 수 없는 타입은 공격으로 간주
		Disconnect(s);
		break;
	}
}

void LoginServer::OnError(int errorcode, const char* msg)
{
	if (msg)
		LOG(L"LoginServer", CSystemLog::LEVEL_ERROR,
		    L"OnError: %d - %S", errorcode, msg);
}

//============================================================
// 패킷 핸들러
//============================================================

void LoginServer::Handle_CS_LOGIN_REQ_LOGIN(SessionKey s, CPacket* pPacket)
{
	// payload 크기 검증
	const int needed = (int)(sizeof(INT64) + 64);
	if (pPacket->GetDataSize() < needed)
	{
		Disconnect(s);
		return;
	}

	DBJob job = {};
	job.type       = eDBJob_Login;
	job.sessionKey = s;
	*pPacket >> job.accountNo;
	pPacket->GetData(job.sessionToken, sizeof(job.sessionToken));

	// IOCP 워커 컨텍스트에서는 DB 호출 금지 -> 큐로 넘김
	_dbJobQueue.Push(job);
}

//============================================================
// DB 워커 스레드
//============================================================

unsigned int __stdcall LoginServer::DBWorkerThread(LPVOID arg)
{
	LoginServer* pServer = (LoginServer*)arg;

	// 이 워커 전용 MYSQL connection 생성
	MYSQL conn;
	mysql_init(&conn);
	if (!mysql_real_connect(&conn,
	                        pServer->_dbHost.c_str(),
	                        pServer->_dbUser.c_str(),
	                        pServer->_dbPasswd.c_str(),
	                        pServer->_dbSchema.c_str(),
	                        pServer->_dbPort, NULL, 0))
	{
		LOG(L"LoginServer", CSystemLog::LEVEL_ERROR,
		    L"DB worker connect failed: %S", mysql_error(&conn));
		return 0;
	}

	pServer->DBWorkerLoop(&conn);

	mysql_close(&conn);
	return 0;
}

void LoginServer::DBWorkerLoop(MYSQL* conn)
{
	while (true)
	{
		DBJob job;
		if (!_dbJobQueue.Pop(job))
		{
			// stop 신호
			break;
		}

		switch (job.type)
		{
		case eDBJob_Login:
			Process_DBJob_Login(conn, job);
			break;
		default:
			LOG(L"LoginServer", CSystemLog::LEVEL_ERROR,
			    L"Unknown DB job type: %d", (int)job.type);
			break;
		}
	}
}

void LoginServer::Process_DBJob_Login(MYSQL* conn, const DBJob& job)
{
	InterlockedIncrement(&_authCount);

	BYTE   Status = dfLOGIN_STATUS_FAIL;
	WCHAR  ID[20] = L"Unknown";
	WCHAR  Nickname[20] = L"NoNick";
	WCHAR  GameServerIP[16] = L"127.0.0.1";
	USHORT GameServerPort   = 12345;
	WCHAR  ChatServerIP[16] = L"127.0.0.1";
	USHORT ChatServerPort   = 21501;
	char   tokenForClient[64] = {};

	char query[512];

	//----------------------------------------------------------
	// 1. 트랜잭션 시작
	//----------------------------------------------------------
	mysql_query(conn, "START TRANSACTION");

	//----------------------------------------------------------
	// 2. account 테이블에서 userid/usernick + token 조회
	//    (token 컬럼은 더미 테스트용. 다음 PR 에서 Redis 로 이관 예정)
	//----------------------------------------------------------
	sprintf_s(query,
	    "SELECT userid, usernick, token FROM account WHERE AccountNo = %lld",
	    job.accountNo);

	bool selectOk = false;
	bool rowFound = false;
	if (mysql_query(conn, query) == 0)
	{
		MYSQL_RES* result = mysql_store_result(conn);
		if (result != NULL)
		{
			MYSQL_ROW row = mysql_fetch_row(result);
			if (row != NULL)
			{
				rowFound = true;

				size_t conv = 0;
				if (mbstowcs_s(&conv, ID,       20, row[0] ? row[0] : "Unknown", _TRUNCATE) != 0)
					wcsncpy_s(ID, L"Unknown", 20);
				if (mbstowcs_s(&conv, Nickname, 20, row[1] ? row[1] : "NoNick", _TRUNCATE) != 0)
					wcsncpy_s(Nickname, L"NoNick", 20);

				if (row[2] != NULL)
				{
					strncpy_s(tokenForClient, sizeof(tokenForClient),
					          row[2], _TRUNCATE);
				}
			}
			mysql_free_result(result);
			selectOk = true;
		}
	}

	if (!selectOk)
	{
		LOG(L"LoginServer", CSystemLog::LEVEL_ERROR,
		    L"SELECT failed (account=%lld): %S",
		    job.accountNo, mysql_error(conn));
		mysql_query(conn, "ROLLBACK");
		Status = dfLOGIN_STATUS_FAIL;
	}
	else if (!rowFound)
	{
		mysql_query(conn, "ROLLBACK");
		Status = dfLOGIN_STATUS_ACCOUNT_MISS;
	}
	else
	{
		//------------------------------------------------------
		// 3. status 갱신 (중복 로그인 방지: status=0 인 경우만 1로)
		//------------------------------------------------------
		sprintf_s(query,
		    "UPDATE status SET status = 1 WHERE AccountNo = %lld AND status = 0",
		    job.accountNo);

		if (mysql_query(conn, query) == 0)
		{
			if (mysql_affected_rows(conn) > 0)
			{
				mysql_query(conn, "COMMIT");
				Status = dfLOGIN_STATUS_OK;
				InterlockedIncrement64(&_totalLoginOK);
			}
			else
			{
				// 이미 게임 중 (status=1) 또는 status row 없음
				mysql_query(conn, "ROLLBACK");
				Status = dfLOGIN_STATUS_GAME;
			}
		}
		else
		{
			LOG(L"LoginServer", CSystemLog::LEVEL_ERROR,
			    L"UPDATE status failed (account=%lld): %S",
			    job.accountNo, mysql_error(conn));
			mysql_query(conn, "ROLLBACK");
			Status = dfLOGIN_STATUS_FAIL;
		}
	}

	if (Status != dfLOGIN_STATUS_OK)
		InterlockedIncrement64(&_totalLoginFail);

	//----------------------------------------------------------
	// 4. 응답 패킷 생성 + 송신
	//----------------------------------------------------------
	CPacket* p = CPacket::Alloc();
	WORD resType = en_PACKET_CS_LOGIN_RES_LOGIN;
	*p << resType;
	*p << (INT64)job.accountNo;
	*p << (BYTE)Status;
	p->PutData((char*)ID,           sizeof(ID));
	p->PutData((char*)Nickname,     sizeof(Nickname));
	p->PutData((char*)GameServerIP, sizeof(GameServerIP));
	*p << (USHORT)GameServerPort;
	p->PutData((char*)ChatServerIP, sizeof(ChatServerIP));
	*p << (USHORT)ChatServerPort;
	// 더미 토큰 전달 (다음 PR 에서 Redis 로 이관)
	p->PutData(tokenForClient, sizeof(tokenForClient));

	bool ret = SendPacket(job.sessionKey, p);
	p->SubRef();

	if (!ret)
	{
		// 보내는 사이 클라가 끊겼음 - 정상 가능. 카운터만 누적, 로그 X
		// (Q3 정책: 응답 못 보내는 경우는 그냥 무시)
	}

	// Q3 결정: 응답 후 즉시 끊기
	// SendPacket 의 OnSend 완료 시점이 아니라 여기서 명시적으로 Disconnect 호출.
	// (CNetServer 가 OnSend 콜백을 따로 노출하지 않으므로,
	//  요청 처리 완료 직후 끊는 것으로 충분. 이미 보낸 패킷은 SendBuf 에 들어있음.)
	Disconnect(job.sessionKey);
}

//============================================================
// 타이머 스레드 - 1초마다 콘솔 출력 + monitor 송신
//============================================================

unsigned int __stdcall LoginServer::TimerThread(LPVOID arg)
{
	LoginServer* pServer = (LoginServer*)arg;

	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

	while (pServer->_bTimerAlive)
	{
		Sleep(1000);
		if (!pServer->_bTimerAlive) break;

		int authTPS = InterlockedExchange(&pServer->_authCount, 0);
		pServer->_authTPS = authTPS;

		int sessionCount   = pServer->GetSessionCount();
		int dbQueueSize    = pServer->_dbJobQueue.GetSize();
		long long totalOK  = pServer->_totalLoginOK;
		long long totalFail= pServer->_totalLoginFail;
		long long totalAcc = pServer->getTotalAcceptCount();
		long invCode       = pServer->getInvalidPacketCodeCount();
		long invLen        = pServer->getInvalidPacketLenCount();
		long decodeFail    = pServer->getDecodeForNetFailCount();
		long sendBufFull   = pServer->getSendBufferFullCount();
		int packetPoolUse  = (int)CPacket::packetPool.GetUseCount();
		int packetPoolCap  = (int)CPacket::packetPool.GetCapacityCount();

		//------------------------------------------------------
		// monitor 송신
		//------------------------------------------------------
		int now = (int)time(NULL);
		pServer->_monitorClient.SendMonitorData(dfMONITOR_DATA_TYPE_LOGIN_SERVER_ON, 1, now);
		pServer->_monitorClient.SendMonitorData(dfMONITOR_DATA_TYPE_LOGIN_SESSION,    sessionCount, now);
		pServer->_monitorClient.SendMonitorData(dfMONITOR_DATA_TYPE_LOGIN_AUTH_TPS,   authTPS, now);
		pServer->_monitorClient.SendMonitorData(dfMONITOR_DATA_TYPE_LOGIN_PACKET_POOL,packetPoolUse, now);

		//------------------------------------------------------
		// 콘솔 출력
		//------------------------------------------------------
		char buf[2048];
		int pos = 0;
		const int W = 70;

		SYSTEMTIME st; GetLocalTime(&st);

		pos += sprintf_s(buf + pos, sizeof(buf) - pos, "%-*s\n", W,
		    "======================================================================");
		pos += sprintf_s(buf + pos, sizeof(buf) - pos, "%-*s\n", W,
		    "                       LoginServer Status");
		char tline[64];
		sprintf_s(tline, "                       %04d-%02d-%02d %02d:%02d:%02d",
		    st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
		pos += sprintf_s(buf + pos, sizeof(buf) - pos, "%-*s\n", W, tline);
		pos += sprintf_s(buf + pos, sizeof(buf) - pos, "%-*s\n", W,
		    "----------------------------------------------------------------------");

		char l[128];
		sprintf_s(l, "  Session: %d / max", sessionCount);
		pos += sprintf_s(buf + pos, sizeof(buf) - pos, "%-*s\n", W, l);

		sprintf_s(l, "  Auth TPS: %d   Total Accept: %lld",
		    authTPS, totalAcc);
		pos += sprintf_s(buf + pos, sizeof(buf) - pos, "%-*s\n", W, l);

		sprintf_s(l, "  Login OK: %lld   Login Fail: %lld",
		    totalOK, totalFail);
		pos += sprintf_s(buf + pos, sizeof(buf) - pos, "%-*s\n", W, l);

		sprintf_s(l, "  DBQueue Size: %d", dbQueueSize);
		pos += sprintf_s(buf + pos, sizeof(buf) - pos, "%-*s\n", W, l);

		sprintf_s(l, "  PacketPool: %d / %d", packetPoolUse, packetPoolCap);
		pos += sprintf_s(buf + pos, sizeof(buf) - pos, "%-*s\n", W, l);

		sprintf_s(l, "  SendBufFull: %ld", sendBufFull);
		pos += sprintf_s(buf + pos, sizeof(buf) - pos, "%-*s\n", W, l);

		sprintf_s(l, "  InvCode:%ld  InvLen:%ld  DecodeFail:%ld",
		    invCode, invLen, decodeFail);
		pos += sprintf_s(buf + pos, sizeof(buf) - pos, "%-*s\n", W, l);

		pos += sprintf_s(buf + pos, sizeof(buf) - pos, "%-*s\n", W,
		    "----------------------------------------------------------------------");

		// 잔상 제거
		for (int i = 0; i < 3; i++)
			pos += sprintf_s(buf + pos, sizeof(buf) - pos, "%-*s\n", W, "");

		COORD origin = { 0, 0 };
		SetConsoleCursorPosition(hConsole, origin);
		DWORD written;
		WriteConsoleA(hConsole, buf, pos, &written, NULL);
	}

	return 0;
}
