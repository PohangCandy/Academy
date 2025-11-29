//---------------------------------------------------------------------------------------------
// 프로젝트명: 
// IO카운트를 통해 세션을 소멸시키는 IOCP 에코 서버 프로젝트
// 
// 목적:
// 1. Overlapped 구조체를 2개 사용해서 Send와 Recv가 병렬성을 가질 수 있도록 만든다.
// 2. 세션의 소멸은 연결 끊김이 아닌, 해당 세션에 대한 모든 IO 작업 완료가 되게 만든다.
// 
// 방법 : 
// 1. Overlapped 구조체를 확장시켜, 완료된 작업에 대한 정보 전달하기(Recv인지 Send인지)
// 2. new로 동적할당한 세션을 맵 형태로 저장.
// 3. 송수신마다 IOCount를 적절히 증감시켜, IOCount가 0되는 세션 삭제시키기
// 
// 결론 :
// 
//---------------------------------------------------------------------------------------------

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma comment(lib, "ws2_32")
#include <WinSock2.h>
#include <stdlib.h>
#include <stdio.h>
#include <map>
#include "errlog.h"
#include "CRingBuffer.h"

#define SERVERPORT 9000
#define BUFSIZE 512



//-------------------------------
// 비동기 입출력 함수 종류
//-------------------------------
enum EIOCP_OPERATION
{
	ERecv,
	ESend
};


//-----------------------------------
// 완료된 비동기 함수를 나타내는 확장된 Overlapped
//-----------------------------------
struct IOCP_CONTEXT {
	OVERLAPPED overlapped = {};
	EIOCP_OPERATION op;

	IOCP_CONTEXT(EIOCP_OPERATION operation)
		:op{operation}
	{

	}

};

//소켓 정보 저장을 위한 구조체
struct SOCKETINFO
{
	IOCP_CONTEXT sendOverlapped{ESend};
	IOCP_CONTEXT recvOverlapped{ERecv};
	SOCKET sock = INVALID_SOCKET;
	CRingBuffer recvBuf{BUFSIZE + 1};
	CRingBuffer sendBuf{BUFSIZE + 1};
	bool IsSending = false;
};

//--------------------------------
// 세션과 세션 ID를 저장하기 위한 맵 
//--------------------------------
std::map<long long, SOCKETINFO*> mSession;
long long SessionID;

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

		//소켓 정보 구조체 할당
		SOCKETINFO* ptr = new SOCKETINFO;
		if (ptr == NULL) break;
		ZeroMemory(&ptr->recvOverlapped, sizeof(ptr->recvOverlapped));
		ptr->recvOverlapped.op = ERecv;
		ptr->sock = client_sock;
		ptr->recvBuf.ClearBuffer();
		WSABUF wsabuf;
		wsabuf.buf = ptr->recvBuf.GetFrontBufferPtr();
		wsabuf.len = ptr->recvBuf.GetFreeSize();

		//세션을 맵에 저장
		//mSession[SessionID++] = ptr;

		//소켓과 입출력 완료 포트 연결
		CreateIoCompletionPort((HANDLE)client_sock, hcp, (ULONG_PTR)ptr, 0);

		//비동기 입출력 시작
		flags = 0;
		retval = WSARecv(client_sock, &wsabuf, 1, &recvbytes, &flags, (LPWSAOVERLAPPED)&ptr->recvOverlapped, NULL);
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
		DWORD cbTransferred;
		SOCKET client_sock;
		SOCKETINFO* ptr;
		IOCP_CONTEXT* lpOverlapped;
		retval = GetQueuedCompletionStatus(hcp, &cbTransferred, (PULONG_PTR)&ptr, (LPOVERLAPPED*)&lpOverlapped, INFINITE);

		//클라이언트 정보 얻기
		SOCKADDR_IN clientaddr;
		int addrlen = sizeof(clientaddr);
		getpeername(ptr->sock, (SOCKADDR*)&clientaddr, &addrlen);

		//비동기 입출력 결과 확인
		if (cbTransferred == 0)
		{
			closesocket(ptr->sock);
			printf("[TCP 서버] 클라이언트 종료 : IP 주소 = %s, 포트번호 = %d\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
			delete ptr;
			continue;
		}
		else if (retval == 0)
		{
			DWORD temp1, temp2;
			WSAGetOverlappedResult(ptr->sock, (LPWSAOVERLAPPED)&lpOverlapped, &temp1, false, &temp2);
			err_display("WSAGetOverlappedResult()");
		}

		//--------------------------------
		// Recv완료 후 처리
		// 1. 링버퍼에 들어온 크기만큼 Rear의 위치 옮기기 -> 링버퍼가 덮어씌워지는 일이 없도록 주의해야 함.
		// 2. Echo이므로 메시지를 읽은 후, 다시 Send, 이후 다시 Recv상태로 전환 
		//--------------------------------
		if (lpOverlapped->op == ERecv)
		{
			if (!ptr->recvBuf.MoveRear(cbTransferred))
			{
				//수신 링버퍼가 가득차서 더이상 데이터를 받을 수 없는 상황
				printf("[SendRingbuf] : I'm Alread Full\n");
			}

			//Recv 버퍼에 있는 내용 읽어서, send링버퍼에 담기
			char buf[BUFSIZE + 1];
			if (cbTransferred != ptr->recvBuf.Dequeue(buf, cbTransferred))
			{
				//수신 링버퍼에 읽을 수 있는 길이만큼 담기지 않았음.
				//100%human error
				printf("human error occur while recv\n");
			}
			
			buf[cbTransferred] = '\0';

			int buflen = strlen(buf);
			if (buflen != ptr->sendBuf.Enqueue(buf, buflen))
			{
				printf("송신 링버퍼가 꽉 참.\n");
				//클라 강제 종료 절차.
			}
			else {
				printf("[TCP/%s : %d] %s\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port), buf); //이부분에서 줄바꿈이 씹힘.
			}


			//Send 중이 아니라면
			//Send 링버퍼에 있는 있는 내용 전부 Send
			if (ptr->IsSending == false)
			{
				ptr->IsSending = true;
				ZeroMemory(&ptr->sendOverlapped, sizeof(ptr->sendOverlapped));
				ptr->sendOverlapped.op = ESend;

				int sendlen = ptr->sendBuf.GetUseSize();
				if (sendlen > ptr->sendBuf.DirectDequeueSize())
				{
					WSABUF wsabuf[2];
					wsabuf[0].buf = ptr->sendBuf.GetFrontBufferPtr();
					wsabuf[0].len = ptr->sendBuf.DirectDequeueSize();
					int frontSize = ptr->sendBuf.GetBufferSize() - ptr->sendBuf.GetFreeSize() - ptr->sendBuf.DirectDequeueSize();
					wsabuf[1].buf = ptr->sendBuf.GetRearBufferPtr() - frontSize;
					wsabuf[1].len = frontSize;

					retval = WSASend(ptr->sock, wsabuf, 2, (LPDWORD)&sendlen, 0, (LPWSAOVERLAPPED)&ptr->sendOverlapped, NULL);
					if (retval == SOCKET_ERROR)
					{
						if (WSAGetLastError() != WSA_IO_PENDING)
						{
							err_display("WSASend()");
						}
						continue;
					}
				}
				else
				{
					WSABUF wsabuf;
					wsabuf.buf = ptr->sendBuf.GetFrontBufferPtr();
					wsabuf.len = ptr->sendBuf.DirectDequeueSize();

					retval = WSASend(ptr->sock, &wsabuf, 1, (LPDWORD)&sendlen, 0, (LPWSAOVERLAPPED)&ptr->sendOverlapped, NULL);
					if (retval == SOCKET_ERROR)
					{
						if (WSAGetLastError() != WSA_IO_PENDING)
						{
							err_display("WSASend()");
						}
						continue;
					}
				}
			}
			
			//Send 한 후 다시 Recv 등록하기
			ZeroMemory(&ptr->recvOverlapped, sizeof(ptr->recvOverlapped));
			ptr->recvOverlapped.op = ERecv;
			WSABUF wsabuf;
			wsabuf.buf = ptr->recvBuf.GetFrontBufferPtr();
			wsabuf.len = ptr->recvBuf.GetFreeSize();

			DWORD recvbytes;
			DWORD flags = 0;
			retval = WSARecv(ptr->sock, &wsabuf, 1, &recvbytes, &flags, (LPWSAOVERLAPPED)&ptr->recvOverlapped, NULL);
			if (retval == SOCKET_ERROR)
			{
				if (WSAGetLastError() != WSA_IO_PENDING) {
					err_display("WSARecv()");
				}
				continue;
			}

		}
		//--------------------------------
		// Send완료 후 처리
		//--------------------------------
		else if(lpOverlapped->op == ESend)
		{
			//SendRingBuffer 정리
			//Send에 성공한 크기만큼 송신 버퍼에서 movefront
			ptr->sendBuf.MoveFront(cbTransferred);
			ptr->IsSending = false;
		}
	}

	return 0;
}
