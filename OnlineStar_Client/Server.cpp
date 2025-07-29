#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>
#include <list>
#include <vector>
#include "Console.h"
#include "CScreenBuffer.h"

#pragma comment(lib, "ws2_32")

#define PORT 3000
#define MAX_BUFFER 1024
#define MAX_ID 10000
#define dfSCREEN_WIDTH  80
#define dfSCREEN_HEIGHT 23

struct stHEADER {
	int Type; // 0: ID 부여, 1: 생성, 2: 삭제, 3: 이동
	int ID;
	int X;
	int Y;
};

struct Player {
	SOCKET sock;
	sockaddr_in addr;
	int ID;
	int X;
	int Y;
	bool bDisconnected = false;
};

std::list<Player*> g_playerList;
int g_nextID = 0;
SOCKET g_listenSock;
int g_frameRecvCount = 0;

void Disconnect(Player* player);
void SendUnicast(Player* player, const stHEADER& msg);
void SendBroadcast(Player* except, const stHEADER& msg);
void AcceptProc();
void RecvProc(Player* player);
void Render();

int main() {
	WSADATA wsa;
	WSAStartup(MAKEWORD(2, 2), &wsa);
	cs_Initial();

	g_listenSock = socket(AF_INET, SOCK_STREAM, 0);
	sockaddr_in servAddr = {};
	servAddr.sin_family = AF_INET;
	servAddr.sin_port = htons(PORT);
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	bind(g_listenSock, (sockaddr*)&servAddr, sizeof(servAddr));
	listen(g_listenSock, SOMAXCONN);

	u_long on = 1;
	ioctlsocket(g_listenSock, FIONBIO, &on);

	while (true) {
		fd_set rset;
		FD_ZERO(&rset);
		FD_SET(g_listenSock, &rset);
		for (auto p : g_playerList) {
			FD_SET(p->sock, &rset);
		}
		int ret = select(0, &rset, nullptr, nullptr, nullptr);
		if (ret < 0) continue;

		if (FD_ISSET(g_listenSock, &rset)) {
			AcceptProc();
		}

		for (auto it = g_playerList.begin(); it != g_playerList.end(); ) {
			Player* p = *it;
			if (FD_ISSET(p->sock, &rset)) {
				RecvProc(p);
				if (p->bDisconnected) {
					closesocket(p->sock);
					delete p;
					it = g_playerList.erase(it);
					continue;
				}
			}
			++it;
		}

		Render();
	}

	WSACleanup();
	return 0;
}

void AcceptProc() {
	sockaddr_in caddr;
	int clen = sizeof(caddr);
	SOCKET cSock = accept(g_listenSock, (sockaddr*)&caddr, &clen);
	if (cSock == INVALID_SOCKET) return;
	u_long on = 1;
	ioctlsocket(cSock, FIONBIO, &on);

	Player* newPlayer = new Player;
	newPlayer->sock = cSock;
	newPlayer->addr = caddr;
	newPlayer->ID = g_nextID++;
	newPlayer->X = dfSCREEN_WIDTH / 2;
	newPlayer->Y = dfSCREEN_HEIGHT / 2;

	g_playerList.push_back(newPlayer);

	// 1. ID 부여
	stHEADER msg;
	msg.Type = 0;
	msg.ID = newPlayer->ID;
	msg.X = newPlayer->X;
	msg.Y = newPlayer->Y;
	SendUnicast(newPlayer, msg);

	// 2. 자기 별 생성 메시지 (새 플레이어에게)
	msg.Type = 1;
	SendUnicast(newPlayer, msg);

	// 3. 기존 플레이어들에게 새 플레이어 생성 통보
	SendBroadcast(newPlayer, msg);

	// 4. 새 플레이어에게 기존 플레이어들 정보 전송
	for (auto p : g_playerList) {
		if (p == newPlayer) continue;
		stHEADER other;
		other.Type = 1;
		other.ID = p->ID;
		other.X = p->X;
		other.Y = p->Y;
		SendUnicast(newPlayer, other);
	}
}

void RecvProc(Player* player) {
	char buf[MAX_BUFFER];
	int ret = recv(player->sock, buf, MAX_BUFFER, 0);
	if (ret <= 0) {
		Disconnect(player);
		return;
	}
	if (ret < sizeof(stHEADER)) return;

	stHEADER* pkt = (stHEADER*)buf;
	if (pkt->Type == 3) { // 이동
		player->X = pkt->X;
		player->Y = pkt->Y;
		SendBroadcast(nullptr, *pkt);
		g_frameRecvCount++;
	}
}

void SendUnicast(Player* player, const stHEADER& msg) {
	int ret = send(player->sock, (const char*)&msg, sizeof(msg), 0);
	if (ret == SOCKET_ERROR) {
		Disconnect(player);
	}
}

void SendBroadcast(Player* except, const stHEADER& msg) {
	for (auto p : g_playerList) {
		if (p == except) continue;
		SendUnicast(p, msg);
	}
}

void Disconnect(Player* player) {
	player->bDisconnected = true;

	stHEADER msg;
	msg.Type = 2; // 삭제
	msg.ID = player->ID;
	msg.X = 0;
	msg.Y = 0;
	SendBroadcast(player, msg);
}

void Render() {
	CScreenBuffer* pBuffer = CScreenBuffer::GetInstance();
	pBuffer->Buffer_Clear();

	char info[80];
	sprintf_s(info, "Connect Client : %zu   Packet : %d", g_playerList.size(), g_frameRecvCount);
	for (int i = 0; info[i] != '\0'; ++i) {
		pBuffer->Sprite_Draw(i, 0, info[i]);
	}

	for (auto& p : g_playerList) {
		pBuffer->Sprite_Draw(p->X, p->Y, '*');
	}

	pBuffer->Buffer_Flip();
	cs_MoveCursor(0, dfSCREEN_HEIGHT);
}