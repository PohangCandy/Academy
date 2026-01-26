//---------------------------------------------------------------------------------------------
// 프로젝트명: 
// shutdown 테스트 
// 
// 목적:
// 서버 컨텐츠에서 클라의 종료를 유도하고자 할 때, 송수신을 막고 IO Count가 0이 되게 유도하는 방법을 찾기 위함.
// shutdown함수를 사용하면 해당 소켓에 대해 더이상 송수신을 진행하지 않는대 그 원리를 알아보기 위함.
// 
// 방법 : 
// 1. 에코를 할 수 있는 클라와 서버를 두고 shutdown 옵션 변경해가며 서버 제어하기
// 2. closesocket과 비교
// 
// 결론 :
// 1. shutdown을 하면 클라에게 FIN 또는 RST를 보내어 송수신 차단
// 2. 해당 소켓에 대한 Recv와 Send 진행 시 소켓 에러 발생하므로 서버 쪽에서도 송수신 불가능
// 3. getsockopt결과를 통해 closesocket과 달리 소켓을 해제하는게 아님을 확인 -> 즉 재할당 발생할 일 없음.
// 
// 추후 예정 :
// IOCP CLanServer에 적용
// 컨텐츠의 shutdown 방식을 통해 네트워크의 세션 해제 유도
//---------------------------------------------------------------------------------------------

#define _WINSOCK_DEPRECATED_NO_WARNINGS

#pragma comment(lib,"ws2_32")
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include "errlog.h"

#define SERVERPORT 9000
#define BUFSIZE 512

int main(int argc, char *argv[])
{
	int retval;

	//윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 1;

	//socket
	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET) err_quit("socket()");

	//bind()
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERVERPORT);
	retval = bind(listen_sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) err_quit("bind()");

	//listen()
	retval = listen(listen_sock, SOMAXCONN);
	if (retval == SOCKET_ERROR) err_quit("listen()");

	//데이터 통신에 사용할 변수
	SOCKET client_sock;
	SOCKADDR_IN clientaddr;
	int addrlen;
	char buf[BUFSIZE + 1];
	//클라리언트 IP정보 담을 버퍼
	char szClientIP[16] = { 0 };

	

	while (1) {
		int countDownShut = 5;
		//accept()
		addrlen = sizeof(clientaddr);
		client_sock = accept(listen_sock, (SOCKADDR*)&clientaddr, &addrlen);
		if (client_sock == INVALID_SOCKET) {
			err_display("accept()");
			break;
		}

		InetNtopA(AF_INET, &clientaddr.sin_addr, szClientIP, 16);

		//접속한 클라이언트 정보 출력
		printf("\n[TCP Server] Client Connect : IP  address = %s, Port Num = %d\n"
			, szClientIP, ntohs(clientaddr.sin_port));

		//클라이언트와 데이터 통신
		while (1) {
			//데이터 받기
			retval = recv(client_sock, buf, BUFSIZE, 0);
			if (retval == SOCKET_ERROR)
			{
				err_display("recv()");
				break;
			}
			else if (retval == 0)
				break;

			//받은 데이터 출력
			buf[retval] = '\0';
			printf("[TCP/ %s : %d]", szClientIP, ntohs(clientaddr.sin_port));
			printf(" %s\n", buf);
			
			if (countDownShut-- == 0)
			{
				//LINGER optval;
				//optval.l_onoff = 1;
				//optval.l_linger = 0;
				//bool bEnable = TRUE;
				//int ret = setsockopt(client_sock, SOL_SOCKET, SO_LINGER, (char*)&optval, sizeof(optval));
				//if (ret == SOCKET_ERROR) err_quit("setsockopt()");
				
				//shutdown(client_sock, SD_SEND);
				shutdown(client_sock, SD_RECEIVE);
				//shutdown(client_sock, SD_BOTH);
				//closesocket(client_sock);

				int optVal;
				int optLen = sizeof(int);

				if (getsockopt(client_sock,
					SOL_SOCKET,
					SO_ACCEPTCONN,
					(char*)&optVal,
					&optLen) != SOCKET_ERROR)
					printf("SockOpt Value: %ld\n", optVal);

			}

			//데이터 보내기
			retval = send(client_sock, buf, retval, 0);
			if (retval == SOCKET_ERROR)
			{
				err_display("send()");
				break;
			}



		}

		//closesocket()
		//closesocket(client_sock);

		printf("[TCP Server] Client Exit: IP address = %s, Port num = %d\n", 
			szClientIP, ntohs(clientaddr.sin_port));
		break;
	}

	//closesocket()                              
	//closesocket(client_sock);
	closesocket(listen_sock);

	//윈속 종료
	WSACleanup();
	return 0;
}