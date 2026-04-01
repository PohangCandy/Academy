#include "stdafx.h"
#include "ChattingServer.h"

//------------------------------------------------------------
// 모니터링 서버 접속 설정
//------------------------------------------------------------
#define MONITOR_SERVER_IP	"127.0.0.1"
#define MONITOR_SERVER_PORT	20000		// LAN 포트
#define CHAT_SERVER_NO		3			// 채팅서버 ServerNo (모니터링용)

int main()
{
	ChattingServer chatserver;

	// 모니터링 서버에 LAN 접속 (Start()가 blocking이므로 먼저 연결)
	if (!chatserver.ConnectMonitor(MONITOR_SERVER_IP, MONITOR_SERVER_PORT, CHAT_SERVER_NO))
	{
		printf("[Main] Warning: Monitor server connection failed. Continuing without monitoring.\n");
	}

	// Start()는 non-blocking (accept가 별도 스레드)
	chatserver.Start(21501, 15000);

	// 메인 스레드 대기
	while (true)
	{
		Sleep(1000);
	}

	return 0;
}
