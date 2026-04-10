#include "stdafx.h"
#include "LoginServer.h"
#include "CPacketForMultiThread.h"
#include "CrashDump.h"
#include "CSystemLog.h"
#include "CConfigParser.h"
#include <conio.h>

int main()
{
	CCrashDump::Init();

	//------------------------------------------------------------
	// 설정 파일 로드
	//------------------------------------------------------------
	CConfigParser config;
	if (!config.Open("ServerConfig.ini"))
	{
		printf("[LoginServer] ServerConfig.ini load failed!\n");
		return 1;
	}

	// 시스템 로그 설정
	const char* logDir      = config.GetString("System", "LOG_DIRECTORY", "Log");
	const char* logLevelStr = config.GetString("System", "LOG_LEVEL", "DEBUG");
	bool consoleOutput      = config.GetBool("System", "CONSOLE_OUTPUT", false);

	wchar_t wLogDir[256];
	MultiByteToWideChar(CP_ACP, 0, logDir, -1, wLogDir, 256);
	SYSLOG_DIRECTORY(wLogDir);

	CSystemLog::en_LOG_LEVEL logLevel = CSystemLog::LEVEL_DEBUG;
	if (_stricmp(logLevelStr, "ERROR") == 0)  logLevel = CSystemLog::LEVEL_ERROR;
	if (_stricmp(logLevelStr, "SYSTEM") == 0) logLevel = CSystemLog::LEVEL_SYSTEM;
	SYSLOG_LEVEL(logLevel);
	CSystemLog::GetInstance()->SetConsoleOutput(consoleOutput);

	// 서버 설정
	int loginPort       = config.GetInt   ("LoginServer", "PORT",            21500);
	int maxSession      = config.GetInt   ("LoginServer", "MAX_SESSION",     5000);
	int netWorkerCount  = config.GetInt   ("LoginServer", "NET_WORKER_COUNT", 0);

	// DB 설정
	const char* dbHost   = config.GetString("DB", "HOST",     "127.0.0.1");
	int         dbPort   = config.GetInt   ("DB", "PORT",     3306);
	const char* dbUser   = config.GetString("DB", "USER",     "root");
	const char* dbPass   = config.GetString("DB", "PASSWORD", "");
	const char* dbSchema = config.GetString("DB", "DATABASE", "accountdb");
	int dbWorkerCount    = config.GetInt   ("DB", "WORKER_COUNT", 4);

	// Monitor 설정
	const char* monitorIP = config.GetString("Monitor", "IP",        "127.0.0.1");
	int monitorPort       = config.GetInt   ("Monitor", "PORT",      20000);
	int serverNo          = config.GetInt   ("Monitor", "SERVER_NO", 1);

	// 풀 상한
	int maxPacketPool = config.GetInt("Pool", "MAX_PACKET_POOL", 20000);
	CPacket::packetPool.SetMaxCapacity(maxPacketPool);

	printf("[LoginServer] Config loaded\n");
	printf("  Port:%d  MaxSession:%d  NetWorker:%d\n",
	    loginPort, maxSession, netWorkerCount);
	printf("  DB %s:%d/%s  DBWorker:%d\n",
	    dbHost, dbPort, dbSchema, dbWorkerCount);
	printf("  Monitor %s:%d  ServerNo:%d\n",
	    monitorIP, monitorPort, serverNo);
	printf("  PacketPool max:%d\n", maxPacketPool);

	LoginServer loginServer;

	if (!loginServer.StartServer(loginPort, maxSession,
	                             netWorkerCount, dbWorkerCount,
	                             dbHost, dbPort, dbUser, dbPass, dbSchema,
	                             monitorIP, monitorPort, serverNo))
	{
		printf("[LoginServer] StartServer failed - check log\n");
		return 1;
	}

	// 메인 스레드 대기 ('q' 키로 Graceful Shutdown)
	while (true)
	{
		if (_kbhit())
		{
			char ch = _getch();
			if (ch == 'q' || ch == 'Q')
				break;
		}
		Sleep(100);
	}

	printf("[LoginServer] Shutting down...\n");
	loginServer.StopServer();
	printf("[LoginServer] Shutdown complete\n");

	return 0;
}
