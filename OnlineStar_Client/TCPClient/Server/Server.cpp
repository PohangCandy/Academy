#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>
#include <list>
#include "Console.h"
#include "CScreenBuffer.h"
#include "errlog.h"
using namespace std;

#pragma comment(lib, "ws2_32")

#define PORT 3000
#define MAX_BUFFER 1600
#define FRAME 100

//#pragma pack(push, 1)
//#pragma pack(pop)

//메시지 타입별 고유번호
enum PacketType {
	ASSIGN_ID = 0,
	CREATE_STAR = 1,
	REMOVE_STAR = 2,
	MOVE = 3,
};

// ID 할당
struct GiveID 
{
	int Type;
	int ID;
	int unused1 = 0;
	int unused2 = 0;
};

// 별 생성
struct CreateStar 
{
	int Type;
	int ID;
	int X;
	int Y;
};

// 별 삭제
struct RemoveStar {
	int Type;
	int ID;
	int unused1 = 0;
	int unused2 = 0;
};

// 별 이동
struct MoveStar {
	int Type;
	int ID;
	int X;
	int Y;
};

struct Player {
	SOCKET sock;
	sockaddr_in addr;
	char IP[16];
	int Port;
	int ID;
	int X;
	int Y;
	bool bDisconnected = false;
};

//플레이어들 담을 리스트 정보
list<Player*> PlayerList;

//다음에 할당할 ID
int NextID = 0;

SOCKET listenSock;

void NetworkProc();
void Render(int displayedFPS);

//다양한 메시지를 처리하기위해 템플릿 적용
template<typename T>
void SendUnicast(Player* player, const T& msg);
template<typename T>
void SendBroadcast(Player* except, const T& msg);

void AcceptProc();
void RecvProc(Player* player);
void Disconnect(Player* player);


int displayedFPS = FRAME;

int main() 
{
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		printf("윈속 초기화 실패");
		return 1;
	}
	cs_Initial();

	//접속 동작 전 예비 동작
	listenSock = socket(AF_INET, SOCK_STREAM, 0);
	sockaddr_in serverAddr = {};
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(PORT);
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	bind(listenSock, (sockaddr*)&serverAddr, sizeof(serverAddr));
	if (listenSock == INVALID_SOCKET) err_quit("bind()");

	listen(listenSock, SOMAXCONN);
	if(listenSock == SOCKET_ERROR) err_quit("listen()");

	//넌블로킹 소켓 세팅
	u_long on = 1;
	ioctlsocket(listenSock, FIONBIO, &on);
	if (listenSock == SOCKET_ERROR) err_quit("ioctlsocket()");


	DWORD lastFrameTime = GetTickCount();
	DWORD lastFPSUpdateTime = GetTickCount();
	int frameCount = 0;

	while (true) {
		DWORD now = GetTickCount();
		//if (now - lastFrameTime < 10) continue;
		//lastFrameTime = now;
		frameCount++;

		//1초마다 프레임 결과를 출력
		if (now - lastFPSUpdateTime >= 1000) {
			displayedFPS = frameCount;
			frameCount = 0;
			lastFPSUpdateTime = now;
		}

		NetworkProc();
		Render(displayedFPS);
	}

	WSACleanup();
	return 0;
}

void NetworkProc() 
{
	fd_set rset;
	FD_ZERO(&rset);
	//접속 동작 감지
	FD_SET(listenSock, &rset);

	//이동,종료 동작 감지
	for (auto p : PlayerList) {
		FD_SET(p->sock, &rset);
	}

	//메시지가 없으면 무한 대기
	int ret = select(0, &rset, nullptr, nullptr, nullptr);
	if (ret == SOCKET_ERROR) err_quit("select()");
	if (ret < 0) return;

	//접속 허락
	if (FD_ISSET(listenSock, &rset)) {
		AcceptProc();
	}

	//이동,종료 허락
	for (auto it = PlayerList.begin(); it != PlayerList.end(); ) {
		Player* p = *it;
		if (FD_ISSET(p->sock, &rset)) {
			RecvProc(p);

			//플래그 비활성화된 플레이어 삭제
			if (p->bDisconnected) {
				closesocket(p->sock);
				delete p;
				it = PlayerList.erase(it);
				continue;
			}
		}
		++it;
	}
}

//플레이어 접속
void AcceptProc() {
	sockaddr_in clientAddr;
	int addlen = sizeof(clientAddr);
	SOCKET clientSock = accept(listenSock, (sockaddr*)&clientAddr, &addlen);
	if (clientSock == INVALID_SOCKET) err_display("accept()");
	u_long on = 1;
	ioctlsocket(clientSock, FIONBIO, &on);

	Player* newPlayer = new Player;
	newPlayer->sock = clientSock;
	newPlayer->addr = clientAddr;
	InetNtopA(AF_INET, &clientAddr.sin_addr, newPlayer->IP, 16);
	newPlayer->Port = clientAddr.sin_port;
	newPlayer->ID = NextID++;
	newPlayer->X = dfSCREEN_WIDTH / 2;
	newPlayer->Y = dfSCREEN_HEIGHT / 2;

	PlayerList.push_back(newPlayer);

	//ID 할당
	GiveID assignMsg = { ASSIGN_ID, newPlayer->ID};
	SendUnicast(newPlayer, assignMsg);

	//새로운 플레이어 별 생성
	CreateStar createMsg = { CREATE_STAR, newPlayer->ID, newPlayer->X, newPlayer->Y };
	SendUnicast(newPlayer, createMsg);

	//기존 플레이어들에게 새로운 플레이어 별 생성
	SendBroadcast(newPlayer, createMsg);

	//새로운 기존에게 기존의 플레이어 정보 전송
	for (auto p : PlayerList) 
	{
		if (p == newPlayer) continue;
		CreateStar other = { CREATE_STAR, p->ID, p->X, p->Y };
		SendUnicast(newPlayer, other);
	}
}

void RecvProc(Player* player) 
{
	char buf[MAX_BUFFER];
	int ret = recv(player->sock, buf, MAX_BUFFER, 0);

	//접속 종료
	if (ret <= 0) {
		Disconnect(player);
		return;
	}

	//별 이동
	int type = *(int*)buf;
	switch (type) {
	case MOVE: {
		if (ret < sizeof(MoveStar)) return;
		MoveStar* pkt = (MoveStar*)(buf);
		player->X = pkt->X;
		player->Y = pkt->Y;
		//플레이어를 제외한 나머지 플레이어에게 메시지 전달
		SendBroadcast(player, *pkt);
		break;
	}
	}
}

//지정된 플레이어에게만 전달
template<typename T>
void SendUnicast(Player* player, const T& msg) 
{
	int ret = send(player->sock, (const char*)(&msg), sizeof(T), 0);
	if (ret == SOCKET_ERROR) {
		Disconnect(player);
	}
}

//지정된 플레이어 제외 전달
template<typename T>
void SendBroadcast(Player* except, const T& msg) 
{
	for (auto p : PlayerList) {
		if (p == except) continue;
		SendUnicast(p, msg);
	}
}

void Disconnect(Player* player) 
{
	//플래그 활성화
	player->bDisconnected = true;

	RemoveStar removeMsg = { REMOVE_STAR, player->ID };
	SendBroadcast(player, removeMsg);
}

void Render(int displayedFPS) 
{
	CScreenBuffer* pBuffer = CScreenBuffer::GetInstance();
	pBuffer->Buffer_Clear();

	char info[80];
	sprintf_s(info, "Connect Client : %zu   Frame : %d", PlayerList.size(), displayedFPS);
	//sprintf_s(info, "Connect Client : %zu", PlayerList.size());
	for (int i = 0; info[i] != '\0'; ++i) {
		pBuffer->Sprite_Draw(i, 0, info[i]);
	}

	for (auto& p : PlayerList) {
		pBuffer->Sprite_Draw(p->X, p->Y, '*');
	}

	pBuffer->Buffer_Flip();
	cs_MoveCursor(0, dfSCREEN_HEIGHT);
}
