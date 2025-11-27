#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma comment(lib, "ws2_32")
#include <WinSock2.h>
#include <stdlib.h>
#include <stdio.h>
#include "errlog.h"

#define SERVERPORT 9000
#define BUFSIZE 512



//소켓 정보 저장을 위한 구조체
struct SOCKETINFO
{
	OVERLAPPED overlapped;
	SOCKET sock;
	char buf[BUFSIZE + 1];
	int recvbytes;
	int sendbytes;
};

//작업자 스레드 함수
DWORD WINAPI WorkerThread(LPVOID arg);

int main(int argc, char* argv[])
{
	int retval;

	//윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 1;

	//입출력 완료 포트 생성
	HANDLE hcp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (hcp == NULL) return 1;

	//CPU 개수 확인
	SYSTEM_INFO si;
	GetSystemInfo(&si);

	//(cpu 개수 * 2)개의 작업자 스레드 생성
	HANDLE hThread;
	for (int i = 0; i < (int)si.dwNumberOfProcessors * 2; i++)
	{
		hThread = CreateThread(NULL, 0, WorkerThread, hcp, 0, NULL);
		if (hThread == NULL) return 1;
		CloseHandle(hThread);
	}

	//socket()
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
	DWORD recvbytes, flags;

	while (1)
	{
		//accept()
		addrlen = sizeof(clientaddr);
		client_sock = accept(listen_sock, (SOCKADDR*)&clientaddr, &addrlen);
		if (client_sock == INVALID_SOCKET) {
			err_display("accept()");
			break;
		}
		printf("[TCP 서버] 클라이언트 접속 : IP 주소 = %s, 포트번호 = %d\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

		//소켓과 입출력 완료 포트 연결
		CreateIoCompletionPort((HANDLE)client_sock, hcp, client_sock, 0);

		//소켓 정보 구조체 할당
		SOCKETINFO* ptr = new SOCKETINFO;
		if (ptr == NULL) break;
		ZeroMemory(&ptr->overlapped, sizeof(ptr->overlapped));
		ptr->sock = client_sock;
		ptr->recvbytes = ptr->sendbytes = 0;
		WSABUF wsabuf;
		wsabuf.buf = ptr->buf;
		wsabuf.len = BUFSIZE;

		//비동기 입출력 시작
		flags = 0;
		retval = WSARecv(client_sock, &wsabuf, 1, &recvbytes, &flags, &ptr->overlapped, NULL);
		if (retval == SOCKET_ERROR)
		{
			if (WSAGetLastError() != ERROR_IO_PENDING) {
				err_display("WSARECV()");
			}
			continue;
		}
	}

	//윈속 종료
	WSACleanup();
	return 0;
}

//작업자 스레드 함수
DWORD __stdcall WorkerThread(LPVOID arg)
{
	int retval;
	HANDLE hcp = (HANDLE)arg;

	while (1) {
		//비동기 입출력 완료 기다리기
		DWORD cbTransferrsd;
		SOCKET client_sock;
		SOCKETINFO* ptr;
		retval = GetQueuedCompletionStatus(hcp, &cbTransferrsd, (PULONG_PTR)&client_sock, (LPOVERLAPPED*)&ptr, INFINITE);

		//클라이언트 정보 얻기
		SOCKADDR_IN clientaddr;
		int addrlen = sizeof(clientaddr);
		getpeername(ptr->sock, (SOCKADDR*)&clientaddr, &addrlen);

		//비동기 입출력 결과 확인
		if (retval == 0 || cbTransferrsd == 0)
		{
			if (retval == 0)
			{
				DWORD temp1, temp2;
				WSAGetOverlappedResult(ptr->sock, &ptr->overlapped, &temp1, false, &temp2);
				err_display("WSAGetOverlappedResult()");
			}
			closesocket(ptr->sock);
			printf("[TCP 서버] 클라이언트 종료 : IP 주소 = %s, 포트번호 = %d\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
			delete ptr;
			continue;
		}

		//데이터 전송량 갱신
		if (ptr->recvbytes == 0)
		{
			ptr->recvbytes = cbTransferrsd;
			ptr->sendbytes = 0;
			//받은 데이터 출력
			ptr->buf[ptr->recvbytes] = '\0';
			printf("[TCP/%s : %d] %s\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port), ptr->buf);
		}
		else
		{
			ptr->sendbytes += cbTransferrsd;
		}

		if (ptr->recvbytes > ptr->sendbytes) {
			//데이터 보내기
			ZeroMemory(&ptr->overlapped, sizeof(ptr->overlapped));
			WSABUF wsabuf;
			wsabuf.buf = ptr->buf + ptr->sendbytes;
			wsabuf.len = ptr->recvbytes - ptr->sendbytes;

			DWORD sendbytes;
			retval = WSASend(ptr->sock, &wsabuf, 1, &sendbytes, 0, &ptr->overlapped, NULL);
			if (retval == SOCKET_ERROR)
			{
				if (WSAGetLastError() != WSA_IO_PENDING)
				{
					err_display("WSASend()");
				}
				continue;
			}
		}
		else {
			ptr->recvbytes = 0;

			//데이터 받기
			ZeroMemory(&ptr->overlapped, sizeof(ptr->overlapped));
			WSABUF wsabuf;
			wsabuf.buf = ptr->buf;
		    wsabuf.len = BUFSIZE;

			DWORD recvbytes;
			DWORD flags = 0;
			retval = WSARecv(ptr->sock, &wsabuf, 1, &recvbytes, &flags, &ptr->overlapped, NULL);
			if (retval == SOCKET_ERROR)
			{
				if (WSAGetLastError() != WSA_IO_PENDING) {
					err_display("WSARecv()");
				}
				continue;
			}
		}
	}

	return 0;
}
