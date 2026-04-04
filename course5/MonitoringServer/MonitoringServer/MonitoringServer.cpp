#include "stdafx.h"
#include <conio.h>
#include "CMonitoringServer.h"
#include "CSystemLog.h"
#include "CConfigParser.h"
#include "CrashDump.h"
#include "CPacketForMultiThread.h"

int main()
{
	CCrashDump::Init();

	//------------------------------------------------------------
	// 설정 파일 로드
	//------------------------------------------------------------
	CConfigParser config;
	if (!config.Open("ServerConfig.ini"))
	{
		printf("[MonitorServer] ServerConfig.ini load failed!\n");
		return 1;
	}

	// 시스템 로그 설정
	const char* logDir = config.GetString("System", "LOG_DIRECTORY", "Log");
	const char* logLevelStr = config.GetString("System", "LOG_LEVEL", "DEBUG");
	bool consoleOutput = config.GetBool("System", "CONSOLE_OUTPUT", false);

	wchar_t wLogDir[256];
	MultiByteToWideChar(CP_ACP, 0, logDir, -1, wLogDir, 256);
	SYSLOG_DIRECTORY(wLogDir);

	CSystemLog::en_LOG_LEVEL logLevel = CSystemLog::LEVEL_DEBUG;
	if (_stricmp(logLevelStr, "ERROR") == 0)  logLevel = CSystemLog::LEVEL_ERROR;
	if (_stricmp(logLevelStr, "SYSTEM") == 0) logLevel = CSystemLog::LEVEL_SYSTEM;
	SYSLOG_LEVEL(logLevel);
	CSystemLog::GetInstance()->SetConsoleOutput(consoleOutput);

	// 서버 설정 읽기
	int lanPort       = config.GetInt("LanServer", "PORT", 20000);
	int lanMaxSession  = config.GetInt("LanServer", "MAX_SESSION", 100);
	int netPort       = config.GetInt("NetServer", "PORT", 21510);
	int netMaxSession  = config.GetInt("NetServer", "MAX_SESSION", 200);

	// DB 설정 읽기
	const char* dbHost = config.GetString("Database", "HOST", "127.0.0.1");
	int dbPort         = config.GetInt("Database", "PORT", 3306);
	const char* dbUser = config.GetString("Database", "USER", "root");
	const char* dbPass = config.GetString("Database", "PASSWORD", "");
	const char* dbName = config.GetString("Database", "DATABASE", "logdb");

	// 풀 상한 설정
	int maxPacketPool = config.GetInt("Pool", "MAX_PACKET_POOL", 10000);
	CPacket::packetPool.SetMaxCapacity(maxPacketPool);

	printf("[MonitorServer] Config loaded - LAN:%d(%d), NET:%d(%d), DB:%s:%d\n",
		lanPort, lanMaxSession, netPort, netMaxSession, dbHost, dbPort);
	printf("[MonitorServer] Pool caps - PacketPool:%d\n", maxPacketPool);

	CMonitoringServer server;

	if (!server.Start(lanPort, lanMaxSession, netPort, netMaxSession,
		dbHost, dbPort, dbUser, dbPass, dbName))
	{
		printf("Server start failed!\n");
		return 1;
	}

	while (1)
	{
		if (_kbhit())
		{
			char ch = _getch();
			if (ch == 'q' || ch == 'Q')
				break;
		}
		Sleep(100);
	}

	server.Stop();
	return 0;
}
