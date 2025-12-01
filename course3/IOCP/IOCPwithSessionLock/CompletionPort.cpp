//---------------------------------------------------------------------------------------------
// 프로젝트명: 
// 한 세션에 하나의 스레드만 접근을 허락하는 IOCP 서버
// 
// 목적:
// IOCount를 없애고 세션 락으로 대체해서 정상작동하도록 만든다.
// 
// 방법 : 
// 1. Session 진입에 CriticalSection을 이용해 다른 스레드의 진입을 막는다.
// 2. SessionMap으로 Session의 키와 Session을 관리한다.
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
#include <process.h>
#include "errlog.h"
#include "CRingBuffer.h"

#define SERVERPORT (6000)
#define BUFSIZE (10000)
#define MSG_SIZE (8)

int d_recv = 0;
int d_send = 0;
int i_recv = 0;
int i_send = 0;

//------------------------------------
//메시지 프로토콜
// 헤더 2Byte (길이)
// 데이터 8Byte(에코)
//------------------------------------
struct Msg {
	short header = 0;
	char payload[MSG_SIZE] = {};
};


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
		:op{ operation }
	{

	}

};

//소켓 정보 저장을 위한 클래스
class SOCKETINFO
{
public:
	SOCKETINFO()
	{
		InitializeCriticalSection(&session_cs);
	}
	~SOCKETINFO()
	{
		DeleteCriticalSection(&session_cs);
	}

	void GetSessionLock()
	{
		EnterCriticalSection(&session_cs);
	}

	void UnLockSession()
	{
		LeaveCriticalSection(&session_cs);
	}

	CRITICAL_SECTION session_cs;
	IOCP_CONTEXT sendOverlapped{ ESend };
	IOCP_CONTEXT recvOverlapped{ ERecv };
	SOCKET sock = INVALID_SOCKET;
	CRingBuffer recvBuf{ BUFSIZE + 1 };
	CRingBuffer sendBuf{ BUFSIZE + 1 };
	LONG IsSending = 0;
	int IOCount = 0;
};

struct IOCPHandle
{
	HANDLE netHcp;
	HANDLE contentHcp;
};

//--------------------------------
// 세션과 세션 ID를 저장하기 위한 맵 
// 걍 배열로 해도 되겠는데?
// 배열 인덱스 번호를 ID로 사용하면 되잖아.
// 근데 이러면 8바이트 index만큼 메모리가 나오려면 메모리 겁나 커지지않을까?
// 맵을 사용하는게 맞겠네
//--------------------------------
std::map<long long, SOCKETINFO*> mSession;
long long g_SessionCounter = 0;

//작업자 스레드 함수
DWORD WINAPI WorkerThread(LPVOID arg);

//컨텐츠 스레드 함수
DWORD WINAPI ContentsThread(LPVOID arg);

//-----------------------------------------
// 세션 종료
// IO가 끝난 세션에 대해 완전히 삭제
//-----------------------------------------
void ReleaseSession(SOCKADDR_IN& clientaddr, SOCKETINFO*& ptr);

int main(int argc, char* argv[])
{
	// ----------------------------------------------------
	// 1. 누수 감지 플래그 및 보고서 모드 설정
	// ----------------------------------------------------
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	// 오류/어설션/경고 보고서를 디버그 출력 창으로 보냄
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
	_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
	// ----------------------------------------------------

	int retval;

	//윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 1;

	//네트워크, 컨텐츠 스레드 입출력 완료 포트 생성
	IOCPHandle* pIOCPHandle = new IOCPHandle;
	pIOCPHandle->netHcp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (pIOCPHandle->netHcp == NULL) return 1;
	pIOCPHandle->contentHcp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (pIOCPHandle->contentHcp == NULL) return 1;

	//CPU 개수 확인
	SYSTEM_INFO si;
	GetSystemInfo(&si);

	//(cpu 개수 * 2)개의 네트워크 작업자 스레드 생성
	HANDLE hThread;
	for (int i = 0; i < (int)si.dwNumberOfProcessors * 2; i++)
		//for (int i = 0; i < 1; i++)
	{
		hThread = CreateThread(NULL, 0, WorkerThread, pIOCPHandle, 0, NULL);
		if (hThread == NULL) return 1;
		CloseHandle(hThread);
	}

	//컨텐츠 스레드 생성
	for (int i = 0; i < (int)si.dwNumberOfProcessors * 2; i++)
		//for (int i = 0; i < 1; i++)
	{
		hThread = CreateThread(NULL, 0, ContentsThread, pIOCPHandle->contentHcp, 0, NULL);
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
		ptr->sendBuf.ClearBuffer();
		WSABUF wsabuf;
		wsabuf.buf = ptr->recvBuf.GetFrontBufferPtr();
		wsabuf.len = ptr->recvBuf.GetFreeSize();

		//세션을 맵에 저장
		mSession[g_SessionCounter++] = ptr;

		//소켓과 입출력 완료 포트 연결
		CreateIoCompletionPort((HANDLE)client_sock, pIOCPHandle->netHcp, (ULONG_PTR)ptr, 0);

		//비동기 입출력 시작
		flags = 0;
		//IOCount를 증가시켰는데 1이라면, 정리중인 세션이므로
		//더 이상 송수신 처리가 일어나지 않도록 한다.
		InterlockedIncrement((long*)&ptr->IOCount);
		InterlockedIncrement((long*)&i_recv);
		retval = WSARecv(client_sock, &wsabuf, 1, &recvbytes, &flags, (LPWSAOVERLAPPED)&ptr->recvOverlapped, NULL);
		if (retval == SOCKET_ERROR)
		{
			if (WSAGetLastError() != ERROR_IO_PENDING) {
				err_display("WSARECV()");
				InterlockedDecrement((long*)&d_recv);
				if (InterlockedDecrement((long*)&ptr->IOCount) == 0)
				{
					ReleaseSession(clientaddr, ptr);
				}
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
	IOCPHandle* iocpHandle = (IOCPHandle*)arg;
	HANDLE hcp = iocpHandle->netHcp;

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
			//클라가 종료신호 FIN보냄.
			printf("[TCP 서버] 클라이언트 종료 신호 수신: IP 주소 = %s, 포트번호 = %d\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
			InterlockedDecrement((long*)&d_recv);
			if (InterlockedDecrement((long*)&ptr->IOCount) == 0)
			{
				ReleaseSession(clientaddr, ptr);
			}
			continue;
		}
		else if (retval == 0)
		{
			DWORD lpcbTransfer, temp2;
			bool isIOSuccess = WSAGetOverlappedResult(ptr->sock, (LPWSAOVERLAPPED)&lpOverlapped, &lpcbTransfer, false, &temp2);
			if (isIOSuccess && lpcbTransfer > 0)
			{
				//GQCS 실패
				err_display("WSAGetOverlappedResult()");
			}
			else
			{
				//IO 실패
				printf("[TCP 서버] IO 실패\n");
				InterlockedDecrement((long*)&d_recv);
				if (InterlockedDecrement((long*)&ptr->IOCount) == 0)
				{
					ReleaseSession(clientaddr, ptr);
				}
				continue;
			}
		}

		int sessionid = -1;
		for (auto& a : mSession)
		{
			if (a.second == ptr) {
				sessionid = a.first;
				break;
			}
		}

		//이제 받은 메시지를 그대로 컨텐츠 스레드로 넘겨주는 것으로 네트워크 스레드의 할 일은 끝
		PostQueuedCompletionStatus(iocpHandle->contentHcp, cbTransferred, sessionid, (LPWSAOVERLAPPED)lpOverlapped);
	}

	return 0;
}


DWORD __stdcall ContentsThread(LPVOID arg)
{
	int retval;
	HANDLE hcp = (HANDLE)arg;

	while (1) {
		//비동기 입출력 완료 기다리기
		DWORD cbTransferred;
		int SessionID;
		IOCP_CONTEXT* lpOverlapped;
		retval = GetQueuedCompletionStatus(hcp, &cbTransferred, (PULONG_PTR)&SessionID, (LPOVERLAPPED*)&lpOverlapped, INFINITE);


		SOCKETINFO* ptr = mSession[SessionID];
		//클라이언트 정보 얻기
		SOCKADDR_IN clientaddr;
		int addrlen = sizeof(clientaddr);
		getpeername(ptr->sock, (SOCKADDR*)&clientaddr, &addrlen);

		//비동기 입출력 결과 확인
		if (cbTransferred == 0)
		{
			//클라가 종료신호 FIN보냄.
			printf("[TCP 서버] 클라이언트 종료 신호 수신: IP 주소 = %s, 포트번호 = %d\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
			InterlockedDecrement((long*)&d_recv);
			if (InterlockedDecrement((long*)&ptr->IOCount) == 0)
			{
				ReleaseSession(clientaddr, ptr);
			}
			continue;
		}
		else if (retval == 0)
		{
			DWORD lpcbTransfer, temp2;
			bool isIOSuccess = WSAGetOverlappedResult(ptr->sock, (LPWSAOVERLAPPED)&lpOverlapped, &lpcbTransfer, false, &temp2);
			if (isIOSuccess && lpcbTransfer > 0)
			{
				//GQCS 실패
				err_display("WSAGetOverlappedResult()");
			}
			else
			{
				//IO 실패
				printf("[TCP 서버] IO 실패\n");
				InterlockedDecrement((long*)&d_recv);
				if (InterlockedDecrement((long*)&ptr->IOCount) == 0)
				{
					ReleaseSession(clientaddr, ptr);
				}
				continue;
			}
		}

		//--------------------------------
		// Recv완료 후 처리
		// 1. 링버퍼에 들어온 크기만큼 Rear의 위치 옮기기 -> 링버퍼가 덮어씌워지는 일이 없도록 주의해야 함.
		// 2. Echo이므로 메시지를 읽은 후, 다시 Send, 이후 다시 Recv상태로 전환 
		//--------------------------------
		if (lpOverlapped->op == ERecv)
		{
			printf("[TCP 서버] 클라이언트 수신, 포트번호 = %d\n", ntohs(clientaddr.sin_port));
			if (!ptr->recvBuf.MoveRear(cbTransferred))
			{
				//수신 링버퍼가 가득차서 더이상 데이터를 받을 수 없는 상황
				printf("[SendRingbuf] : I'm Alread Full\n");
			}

			//Recv 버퍼에 있는 내용 읽어서, send링버퍼에 담기
			Msg recvMsg;

			//송신 링버퍼에 있는 데이터 중 헤더 길이만큼 있는 메시지는 모두 읽어서 처리
			while (1)
			{
				//------------------------------------------------------
				// 1. 링 버퍼에서 Dequeue
				// 먼저 메시지 길이만큼 읽을 후, 해당 메시지 길이를 Dequeue
				//------------------------------------------------------
				short Header_size = ptr->recvBuf.Peek((char*)&recvMsg.header, sizeof(recvMsg.header));
				if (Header_size == sizeof(recvMsg.header) && recvMsg.header != 0)
				{
					if (ptr->recvBuf.GetUseSize() >= recvMsg.header + sizeof(recvMsg.header))
					{
						//이미 읽은 헤더는 제외하고 읽자.
						ptr->recvBuf.MoveFront(sizeof(recvMsg.header));
						int dequeued_size = ptr->recvBuf.Dequeue((char*)&recvMsg.payload, recvMsg.header);

						if (dequeued_size != recvMsg.header)
						{
							printf("[CONSUMER ERROR] 부분 데이터 수신 오류: 요청 %d, 실제 %d\n", recvMsg.header, dequeued_size);
						}
					}
					else
					{
						//헤더가 나타내는 데이터 크기가 다 도착하지 않았음.
						break;
					}

					//메시지를 다시 send링버퍼에 enqueue
					//short buflen = recvMsg.header;
					int buflen = sizeof(recvMsg);
					if (buflen != ptr->sendBuf.Enqueue((char*)&recvMsg, buflen))
					{
						printf("송신 링버퍼가 꽉 참., 포트번호 = %d\n", ntohs(clientaddr.sin_port));
						//클라 강제 종료 절차.
					}
					else {
						printf("[TCP/%s : %d] ", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
						printf("%lld", (long long)recvMsg.payload);
						//for (int i = 0; i < buflen; i++)
						//{
						//	printf("%c", recvMsg.payload[i]);
						//}
						printf("\n");
					}
				}
				else
				{
					//헤더만큼의 데이터도 도착하지 않은 경우 or 데이터의 길이가 0인 경우
					break;
				}
			}


			//Send 중이 아니라면
			//Send 링버퍼에 있는 있는 내용 전부 Send
			if (InterlockedCompareExchange(&ptr->IsSending, 1, 0) == 0)
			{
				printf("송신 진행 중 아님, 송신 루트 탐., 포트번호 = %d\n", ntohs(clientaddr.sin_port));
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
					InterlockedIncrement((long*)&ptr->IOCount);
					InterlockedIncrement((long*)&i_send);
					retval = WSASend(ptr->sock, wsabuf, 2, (LPDWORD)&sendlen, 0, (LPWSAOVERLAPPED)&ptr->sendOverlapped, NULL);


					if (retval == SOCKET_ERROR)
					{
						if (WSAGetLastError() != WSA_IO_PENDING)
						{
							err_display("WSASend()");
							InterlockedDecrement((long*)&d_send);
							if (InterlockedDecrement((long*)&ptr->IOCount) == 0)
							{
								ReleaseSession(clientaddr, ptr);
							}
						}
						continue;
					}
				}
				else
				{
					WSABUF wsabuf;
					wsabuf.buf = ptr->sendBuf.GetFrontBufferPtr();
					wsabuf.len = ptr->sendBuf.DirectDequeueSize();
					InterlockedIncrement((long*)&ptr->IOCount);
					InterlockedIncrement((long*)&i_send);
					retval = WSASend(ptr->sock, &wsabuf, 1, (LPDWORD)&sendlen, 0, (LPWSAOVERLAPPED)&ptr->sendOverlapped, NULL);


					if (retval == SOCKET_ERROR)
					{
						if (WSAGetLastError() != WSA_IO_PENDING)
						{
							err_display("WSASend()");
							InterlockedDecrement((long*)&d_send);
							if (InterlockedDecrement((long*)&ptr->IOCount) == 0)
							{
								ReleaseSession(clientaddr, ptr);
							}
						}
						//-------------------------------------------------
						// 이럴땐 어떻게 하는게 정상일까
						//-------------------------------------------------
						continue;
					}
				}
			}
			else
			{
				printf("송신 진행 중.., 포트번호 = %d\n", ntohs(clientaddr.sin_port));
			}

			//Send 한 후 다시 Recv 등록하기
			printf("다시 recv 대기하기, 포트번호 = %d\n", ntohs(clientaddr.sin_port));
			ZeroMemory(&ptr->recvOverlapped, sizeof(ptr->recvOverlapped));
			ptr->recvOverlapped.op = ERecv;

			//Direct로 넣을 수 있냐 없냐에 따라 여러 버퍼로 나눠서 받아야 함.
			int recvlen = ptr->recvBuf.GetFreeSize();
			if (recvlen > ptr->recvBuf.DirectEnqueueSize())
			{
				WSABUF wsabuf[2];
				wsabuf[0].buf = ptr->recvBuf.GetRearBufferPtr();
				wsabuf[0].len = ptr->recvBuf.DirectEnqueueSize();
				int frontSize = ptr->recvBuf.GetFreeSize() - ptr->recvBuf.DirectEnqueueSize();
				wsabuf[1].buf = ptr->recvBuf.GetFrontBufferPtr() - frontSize;
				wsabuf[1].len = frontSize;
				DWORD recvbytes;
				DWORD flags = 0;
				InterlockedIncrement((long*)&ptr->IOCount);
				InterlockedIncrement((long*)&i_recv);
				retval = WSARecv(ptr->sock, wsabuf, 2, &recvbytes, &flags, (LPWSAOVERLAPPED)&ptr->recvOverlapped, NULL);

				if (retval == SOCKET_ERROR)
				{
					if (WSAGetLastError() != WSA_IO_PENDING)
					{
						err_display("WSARecv()");
					}
					InterlockedDecrement((long*)&d_recv);
					if (InterlockedDecrement((long*)&ptr->IOCount) == 0)
					{
						ReleaseSession(clientaddr, ptr);
					}
					continue;
				}

			}
			else
			{
				WSABUF wsabuf;
				wsabuf.buf = ptr->recvBuf.GetRearBufferPtr();
				wsabuf.len = ptr->recvBuf.DirectEnqueueSize();
				DWORD recvbytes;
				DWORD flags = 0;
				InterlockedIncrement((long*)&ptr->IOCount);
				InterlockedIncrement((long*)&i_recv);
				retval = WSARecv(ptr->sock, &wsabuf, 1, &recvbytes, &flags, (LPWSAOVERLAPPED)&ptr->recvOverlapped, NULL);

				if (retval == SOCKET_ERROR)
				{
					if (WSAGetLastError() != WSA_IO_PENDING)
					{
						err_display("WSARecv()");
					}
					//-------------------------------------------------
					// 이럴땐 어떻게 하는게 정상일까
					//-------------------------------------------------
					InterlockedDecrement((long*)&d_recv);
					if (InterlockedDecrement((long*)&ptr->IOCount) == 0)
					{
						ReleaseSession(clientaddr, ptr);
					}
					continue;
				}
			}

			InterlockedDecrement((long*)&d_recv);
			if (InterlockedDecrement((long*)&ptr->IOCount) == 0)
			{
				ReleaseSession(clientaddr, ptr);
			}
		}

		//--------------------------------
		// Send완료 후 처리
		//--------------------------------
		else if (lpOverlapped->op == ESend)
		{
			printf("[TCP 서버] 클라이언트 송신완료, 포트번호 = %d\n", ntohs(clientaddr.sin_port));

			//락 풀기전에 Send한 크기만큼 송신 버퍼에서 movefront
			ptr->sendBuf.MoveFront(cbTransferred);

			//SendRingBuffer 정리
			if (InterlockedCompareExchange(&ptr->IsSending, 0, 1) == 0)
			{
				printf("Send 중첩 발생, 포트번호 = %d\n", ntohs(clientaddr.sin_port));
			}

			//이때 송신 링버퍼에 데이터가 있다면 다시 Send를 실행해준다.
			//Send 링버퍼에 있는 있는 내용 전부 Send
			if (ptr->sendBuf.GetUseSize() != 0 && InterlockedCompareExchange(&ptr->IsSending, 1, 0) == 0)
			{
				printf("송신 완료 했는데 송신 링버퍼에 잔여물 남은 경우, 포트번호 = %d\n", ntohs(clientaddr.sin_port));
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
					InterlockedIncrement((long*)&ptr->IOCount);
					InterlockedIncrement((long*)&i_send);
					retval = WSASend(ptr->sock, wsabuf, 2, (LPDWORD)&sendlen, 0, (LPWSAOVERLAPPED)&ptr->sendOverlapped, NULL);


					if (retval == SOCKET_ERROR)
					{
						if (WSAGetLastError() != WSA_IO_PENDING)
						{
							err_display("WSASend()");
							InterlockedDecrement((long*)&d_send);
							if (InterlockedDecrement((long*)&ptr->IOCount) == 0)
							{
								ReleaseSession(clientaddr, ptr);
							}
						}
						continue;
					}
				}
				else
				{
					WSABUF wsabuf;
					wsabuf.buf = ptr->sendBuf.GetFrontBufferPtr();
					wsabuf.len = ptr->sendBuf.DirectDequeueSize();
					InterlockedIncrement((long*)&ptr->IOCount);
					InterlockedIncrement((long*)&i_send);
					retval = WSASend(ptr->sock, &wsabuf, 1, (LPDWORD)&sendlen, 0, (LPWSAOVERLAPPED)&ptr->sendOverlapped, NULL);


					if (retval == SOCKET_ERROR)
					{
						if (WSAGetLastError() != WSA_IO_PENDING)
						{
							err_display("WSASend()");
							InterlockedDecrement((long*)&d_send);
							if (InterlockedDecrement((long*)&ptr->IOCount) == 0)
							{
								ReleaseSession(clientaddr, ptr);
							}
						}
						//-------------------------------------------------
						// 이럴땐 어떻게 하는게 정상일까
						//-------------------------------------------------
						continue;
					}
				}
			}



			InterlockedDecrement((long*)&d_send);
			if (InterlockedDecrement((long*)&ptr->IOCount) == 0)
			{
				ReleaseSession(clientaddr, ptr);
			}

		}
	}

	return 0;
}

void ReleaseSession(SOCKADDR_IN& clientaddr, SOCKETINFO*& ptr)
{
	closesocket(ptr->sock);
	printf("[TCP 서버] 클라이언트 종료: IP 주소 = %s, 포트번호 = %d\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
	delete ptr;

	_CrtDumpMemoryLeaks();
}