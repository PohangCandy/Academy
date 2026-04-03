#include "stdafx.h"
#include <conio.h>
#include "CMonitoringServer.h"
#include "CSystemLog.h"
#include "CrashDump.h"

int main()
{
	CCrashDump::Init();
	SYSLOG_DIRECTORY(L"Log");
	SYSLOG_LEVEL(CSystemLog::LEVEL_DEBUG);
	CSystemLog::GetInstance()->SetConsoleOutput(false);

	CMonitoringServer server;

	if (!server.Start())
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
