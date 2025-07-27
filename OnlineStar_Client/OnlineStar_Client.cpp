#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <map>
#include <chrono>
#include <thread>
#include "Console.h"
#include "CScreenBuffer.h"

using namespace std;

#pragma comment(lib, "ws2_32")

#define dfSCREEN_WIDTH  80
#define dfSCREEN_HEIGHT 25

struct stHEADER {
	int Type;
	int ID;
	int X;
	int Y;
};

struct PLAYER {
	int ID;
	int X, Y;
};

map<int, PLAYER> g_players;
int MYID = -1;
SOCKET g_sock;
DWORD g_lastMoveTime = 0;
int g_frameRecvCount = 0;

void InputProc() {
	DWORD now = GetTickCount();
	if (now - g_lastMoveTime < 10) return;

	if (GetAsyncKeyState(VK_LEFT) & 0x8000 ||
		GetAsyncKeyState(VK_RIGHT) & 0x8000 ||
		GetAsyncKeyState(VK_UP) & 0x8000 ||
		GetAsyncKeyState(VK_DOWN) & 0x8000) {

		PLAYER& me = g_players[MYID];
		int nx = me.X, ny = me.Y;

		if (GetAsyncKeyState(VK_LEFT) & 0x8000)  nx = max(0, nx - 1);
		if (GetAsyncKeyState(VK_RIGHT) & 0x8000) nx = min(dfSCREEN_WIDTH - 2, nx + 1);
		if (GetAsyncKeyState(VK_UP) & 0x8000)    ny = max(1, ny - 1);
		if (GetAsyncKeyState(VK_DOWN) & 0x8000)  ny = min(dfSCREEN_HEIGHT - 2, ny + 1);

		if (nx != me.X || ny != me.Y) {
			me.X = nx; me.Y = ny;
			stHEADER pkt = { 3, MYID, nx, ny };
			int sret = send(g_sock, (char*)&pkt, sizeof(pkt), 0);
			if (sret == SOCKET_ERROR) {
				cs_MoveCursor(0, 24);
				printf("[오류] send 실패: %d\n", WSAGetLastError());
			}
			g_lastMoveTime = now;
		}
	}
}

void LogicProc() {
	char buf[1600];
	fd_set rset;
	FD_ZERO(&rset);
	FD_SET(g_sock, &rset);
	timeval timeout = { 0, 0 };
	int ret = select(0, &rset, NULL, NULL, &timeout);
	if (ret == SOCKET_ERROR) {
		cs_MoveCursor(0, 24);
		printf("[오류] select 실패: %d\n", WSAGetLastError());
		return;
	}
	g_frameRecvCount = 0;
	if (ret > 0 && FD_ISSET(g_sock, &rset)) {
		int recvlen = recv(g_sock, buf, 1600, 0);
		if (recvlen <= 0) {
			int err = WSAGetLastError();
			if (err != WSAEWOULDBLOCK) {
				cs_MoveCursor(0, 24);
				printf("[오류] recv 실패 또는 연결 종료: %d\n", err);
			}
			return;
		}
		int offset = 0;
		while (recvlen - offset >= 16) {
			stHEADER* p = (stHEADER*)&buf[offset];
			switch (p->Type) {
			case 0: MYID = p->ID; break;
			case 1: g_players[p->ID] = { p->ID, p->X, p->Y }; break;
			case 2: g_players.erase(p->ID); break;
			case 3:
				if (p->ID != MYID)
					g_players[p->ID] = { p->ID, p->X, p->Y };
				break;
			default:
				cs_MoveCursor(0, 24);
				printf("[경고] 알 수 없는 패킷 타입 수신: %d\n", p->Type);
				break;
			}
			offset += 16;
			++g_frameRecvCount;
		}
	}
}

void RenderProc() {
	CScreenBuffer* pBuffer = CScreenBuffer::GetInstance();
	pBuffer->Buffer_Clear();

	// 접속한 클라이언트 수 & 수신 패킷 수 출력
	char info[80];
	sprintf_s(info, "Connect Client : %zu   Packet : %d", g_players.size(), g_frameRecvCount);
	for (int i = 0; info[i] != '\0'; ++i) {
		pBuffer->Sprite_Draw(i, 0, info[i]);
	}

	// 플레이어 출력
	for (auto it = g_players.begin(); it != g_players.end(); ++it) {
		const PLAYER& pl = it->second;
		pBuffer->Sprite_Draw(pl.X, pl.Y, '*');
	}

	pBuffer->Buffer_Flip();
	cs_MoveCursor(0, 24);
}

int main() {
	cs_Initial(); // 콘솔 초기화 (커서 숨기기 포함)

	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
		printf("[오류] WSAStartup 실패\n");
		return 1;
	}
	g_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (g_sock == INVALID_SOCKET) {
		printf("[오류] 소켓 생성 실패: %d\n", WSAGetLastError());
		WSACleanup();
		return 1;
	}

	char ipStr[32];
	printf("접속할 IP를 입력하세요: ");
	scanf_s("%31s", ipStr, (unsigned)_countof(ipStr));

	sockaddr_in servAddr = {};
	servAddr.sin_family = AF_INET;
	servAddr.sin_port = htons(3000);
	if (InetPtonA(AF_INET, ipStr, &servAddr.sin_addr) != 1) {
		printf("[오류] 잘못된 IP 형식\n");
		closesocket(g_sock);
		WSACleanup();
		return 1;
	}

	if (connect(g_sock, (sockaddr*)&servAddr, sizeof(servAddr)) == SOCKET_ERROR) {
		printf("[오류] 서버 연결 실패: %d\n", WSAGetLastError());
		closesocket(g_sock);
		WSACleanup();
		return 1;
	}

	u_long on = 1;
	ioctlsocket(g_sock, FIONBIO, &on);

	while (1) {
		InputProc();
		LogicProc();
		RenderProc();
		this_thread::sleep_for(chrono::milliseconds(16)); // 60 FPS
	}

	closesocket(g_sock);
	WSACleanup();
	return 0;
}
