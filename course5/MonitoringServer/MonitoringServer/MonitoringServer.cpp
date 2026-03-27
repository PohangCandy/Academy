#include "stdafx.h"
#include <conio.h>
#include "CMonitoringServer.h"
#include "CSystemLog.h"

int main()
{
	CSystemLog::GetInstance()->SetLogLevel(CSystemLog::LEVEL_DEBUG);

	printf("=== Monitoring Server ===\n\n");

	CMonitoringServer server;

	if (!server.Start())
	{
		printf("Server start failed!\n");
		return 1;
	}

	printf("\nPress 'q' to quit.\n\n");

	while (1)
	{
		char ch = _getch();
		if (ch == 'q' || ch == 'Q')
			break;
	}

	server.Stop();
	printf("Server stopped.\n");
	return 0;
}
