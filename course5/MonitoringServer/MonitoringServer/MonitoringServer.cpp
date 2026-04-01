#include "stdafx.h"
#include <conio.h>
#include "CMonitoringServer.h"
#include "CSystemLog.h"

int main()
{
	CSystemLog::GetInstance()->SetLogLevel(CSystemLog::LEVEL_DEBUG);

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
