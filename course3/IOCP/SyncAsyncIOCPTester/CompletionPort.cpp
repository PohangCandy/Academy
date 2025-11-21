//---------------------------------------------------------------------------------------------
// 프로젝트명: 
// IOCP 를 사용한 에코 서버 성능 측정 프로젝트
// 
// 목적:
// IOCP를 사용했을 때 동기, 비동기 통신에 대한 성능 비교
// 
// 방법 : 
// 송수신 데이터의 양에 변화를 주며 동기와 비동기 일때의 Send, Recv 시간 측정
// 
// 결론 :
// 적은 데이터를 보낼 땐, TCP 버퍼에 복사하는 행위(fastIO)가 페이지에 락 걸고 접근하는 것보다 빨라
// 비동기 방식보다 동기 방식이 더 빠름.
// 크기가 큰 데이터를 보낼 땐, 복사하는 행위보다 페이지 락 걸고, 복사없이 접근하는 방식(DirectIO)이 더 빨라
// 동기보다 비동기 방식이 더 빠름.
//---------------------------------------------------------------------------------------------

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma comment(lib, "ws2_32")
#include <WinSock2.h>
#include <stdlib.h>
#include <stdio.h>
#include "errlog.h"
#include "Profile.h"

#define SERVERPORT 9000
#define BUFSIZE (1024 * 1024)

#define MAX_WORKER_THREADS 8 // 실제 si.dwNumberOfProcessors * 2 로 계산되는 값

//send, recv 총 횟수와 오류 횟수를 기록할 변수
int TotalSend = 0;
int TotalRecv = 0;
int errSend = 0;
int errRecv = 0;

//소켓 정보 저장을 위한 구조체
struct SOCKETINFO
{
	OVERLAPPED overlapped;
	SOCKET sock;
	char buf[BUFSIZE + 1];
	int recvbytes;
	int sendbytes;
	//멤버로 굳이 넣지 않아도 됨
	WSABUF wsabuf;
};

//작업자 스레드 함수
DWORD WINAPI WorkerThread(LPVOID arg);

//--------------------------------------------------------
//프로파일링용 함수 이름
//--------------------------------------------------------
WCHAR f1[] = L"WSASend with IO Pending";
WCHAR f2[] = L"GetQueuedCompletionStatus with IO Pending";

//WCHAR f1[] = L"WSASend";
//WCHAR f2[] = L"GetQueuedCompletionStatus";

//----------------------------------------------------------
// 각 스레드마다 카운팅 한 횟수를 저장할 인덱스
//----------------------------------------------------------
DWORD g_dwSendCountTlsIndex = TLS_OUT_OF_INDEXES;

// 각 스레드의 최종 send 횟수를 저장할 배열
int g_arThreadSendCounts[MAX_WORKER_THREADS] = { 0 };

// 스레드 ID를 할당할 카운터 (원자적 연산 필요)
volatile LONG g_lThreadIDCounter = 0;

int main(int argc, char* argv[])
{
	int retval;

	//윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 1;

	//TLS 인덱스 할당
	g_dwSendCountTlsIndex = TlsAlloc();
	if (g_dwSendCountTlsIndex == TLS_OUT_OF_INDEXES) return 1;

	//입출력 완료 포트 생성
	HANDLE hcp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 2);
	if (hcp == NULL) return 1;

	//CPU 개수 확인
	SYSTEM_INFO si;
	GetSystemInfo(&si);

	//(cpu 개수 * 2)개의 작업자 스레드 생성
	HANDLE hThread;
	//for (int i = 0; i < (int)si.dwNumberOfProcessors * 2; i++)
	//for (int i = 0; i < (int)si.dwNumberOfProcessors; i++)
	for (int i = 0; i < 1; i++)
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

	//-----------------------------------------------------
	// 비동기 처리를 위한 IO_pending 유도하기
	//송신 버퍼의 크기 0으로 만들기
	//-----------------------------------------------------
	int optval, optlen;
	optlen = sizeof(optval);
	retval = getsockopt(listen_sock, SOL_SOCKET, SO_SNDBUF, (char*)&optval, &optlen);
	if (retval == SOCKET_ERROR) err_quit("getsockopt()");
	//printf("수신 버퍼 크기(old) = %d바이트\n", optval);
	optval = 0;
	retval = setsockopt(listen_sock, SOL_SOCKET, SO_SNDBUF, (char*)&optval, sizeof(optlen));
	if (retval == SOCKET_ERROR) err_quit("setsockopt()");
	//printf("수신 버퍼 크기(old) = %d바이트\n", optval);
	optlen = sizeof(optval);
	retval = getsockopt(listen_sock, SOL_SOCKET, SO_SNDBUF, (char*)&optval, &optlen);
	if (retval == SOCKET_ERROR) err_quit("getsockopt()");
	//printf("수신 버퍼 크기(old) = %d바이트\n", optval);

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
		ptr->wsabuf.buf = ptr->buf;
		ptr->wsabuf.len = BUFSIZE;

		//비동기 입출력 시작
		flags = 0;
		retval = WSARecv(client_sock, &ptr->wsabuf, 1, &recvbytes, &flags, &ptr->overlapped, NULL);
		if (retval == SOCKET_ERROR)
		{
			if (WSAGetLastError() != ERROR_IO_PENDING) {
				err_display("WSARECV()");
			}
			continue;
		}
	}

	//printf("----IOCP 서버 데이터 총 송수신 횟수----\n");
	//printf("[총 송신] : %d , [pending 발생 횟수] : %d\n", TotalSend, errSend);
	//printf("[총 수신] : %d , [pending 발생 횟수] : %d\n", TotalRecv, errRecv);

	//윈속 종료
	WSACleanup();
	
	return 0;
}

//작업자 스레드 함수
DWORD __stdcall WorkerThread(LPVOID arg)
{
	//스레드 로컬 카운터 초기화 (TLS 슬롯에 0을 저장)
	TlsSetValue(g_dwSendCountTlsIndex, (LPVOID)0);

	//스레드 ID 할당 (0부터 시작)
	int threadID = InterlockedIncrement(&g_lThreadIDCounter) - 1;

	int retval;
	HANDLE hcp = (HANDLE)arg;

	while (1) {
		//비동기 입출력 완료 기다리기
		DWORD cbTransferrsd;
		SOCKET client_sock;
		SOCKETINFO* ptr;

		ProfileBegin(f2);
		retval = GetQueuedCompletionStatus(hcp, &cbTransferrsd, (PULONG_PTR)&client_sock, (LPOVERLAPPED*)&ptr, INFINITE);
		ProfileEnd(f2);

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
			//------------------------------------------------
			//프로파일링 출력을 위한 break
			//------------------------------------------------
			//break;

			DWORD dwFinalCount = (DWORD)TlsGetValue(g_dwSendCountTlsIndex);
			g_arThreadSendCounts[threadID] = dwFinalCount;

			ProfileDataOutText(L"Profile.txt");
			continue;
		}

		//데이터 전송량 갱신
		if (ptr->recvbytes == 0)
		{
			ptr->recvbytes = cbTransferrsd;
			ptr->sendbytes = 0;
			//받은 데이터 출력
			ptr->buf[ptr->recvbytes] = '\0';
			printf("[TCP/%s : %d] %s \n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port), ptr->buf);
		}
		else
		{
			ptr->sendbytes += cbTransferrsd;
		}

		if (ptr->recvbytes > ptr->sendbytes) {
			//데이터 보내기
			ZeroMemory(&ptr->overlapped, sizeof(ptr->overlapped));
			ptr->wsabuf.buf = ptr->buf + ptr->sendbytes;
			ptr->wsabuf.len = ptr->recvbytes - ptr->sendbytes;

			DWORD sendbytes;

			ProfileBegin(f1);
			retval = WSASend(ptr->sock, &ptr->wsabuf, 1, &sendbytes, 0, &ptr->overlapped, NULL);
			ProfileEnd(f1);

			InterlockedAdd((LONG*)&TotalSend, 1);
			if (retval == SOCKET_ERROR)
			{
				if (WSAGetLastError() != WSA_IO_PENDING)
				{
					err_display("WSASend()");
				}
				InterlockedAdd((LONG*)&errSend, 1);
				continue;
			}

			//각 스레드의 send 횟수 측정
			//TlsGetValue로 현재 카운터 값을 가져와 1 증가 후 다시 저장
			DWORD dwCurrentCount = (DWORD)TlsGetValue(g_dwSendCountTlsIndex);
			TlsSetValue(g_dwSendCountTlsIndex, (LPVOID)(dwCurrentCount + 1));
		}
		else {
			ptr->recvbytes = 0;

			//데이터 받기
			ZeroMemory(&ptr->overlapped, sizeof(ptr->overlapped));
			ptr->wsabuf.buf = ptr->buf;
			ptr->wsabuf.len = BUFSIZE;

			DWORD recvbytes;
			DWORD flags = 0;
			retval = WSARecv(ptr->sock, &ptr->wsabuf, 1, &recvbytes, &flags, &ptr->overlapped, NULL);
			InterlockedAdd((LONG*)&TotalRecv, 1);
			if (retval == SOCKET_ERROR)
			{
				if (WSAGetLastError() != WSA_IO_PENDING) {
					err_display("WSARecv()");
				}
				InterlockedAdd((LONG*)&errRecv, 1);
				continue;
			}
		}
	}

	//ProfileDataOutText(L"Profile.txt");

	return 0;
}
