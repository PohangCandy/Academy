//---------------------------------------------------------------------------------------------
// 프로젝트명: 
// 컨텐츠와 네트워크를 완전히 분리한 IOCP 서버
// 
// 목적:
// 다양한 컨텐츠에 활용될 수 있는 네트워크 라이브러리를 설계한다. 
// 
// 방법 : 
// 컨텐츠 스레드를 네트워크 스레드를 분리하고,
// 컨텐츠에서 알 수 있는 네트워크 정보를 세션 ID로 제한한다.
// 락을 통해 컨텐츠 스레드와 네트워크 스레드의 공유 자원을 동기화 시킨다.
// 
// 결론 :
// 1. Send가 중첩으로 발생되지 않기 위한 추가 장치가 마련되어야 했음.
//   -> 송신 링버퍼 락
// 
// 2. 끊어질 세션에 대해 IO가 추가적으로 진행되지 않도록 해야 함.
//  -> IOCount 0(closesocket) -> 1(wsasend) -> 0(send 완료)되면 ReleaseSession을 2번 발생시킴.
// 이를 방지하기 위해 send 증가시킨 결과가 1이라면 send 하지 않도록 만듬.
// 
// 3. 세션 별로 메시지 큐 할당
// -> 전역에 있는 메시지 큐에 넣는 순서가 바뀌었을때, 잘못된 세션에게 메시지를 보낼 수도 있음.
// 
// 추후 예정 :
// 1. 각종 성능 테스트 비교
// 2.  IOCount에 걸린 락 없애고 정상작동 되도록 만들기
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
#define BUFSIZE (1024 * 1024)
#define MSG_SIZE (8)

int d_recv = 0;
int d_send = 0;
int i_recv = 0;
int i_send = 0;
int d_gqcs_client = 0;
int d_gqcs_IO = 0;
int d_gqcs = 0;

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
	ESend,
	//EContents,
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

//------------------------------------
// 메시지 큐
// 워커 스레드가 수신한 메시지를 락을 걸고 메시지 큐에 던져 놓으면
// 컨텐츠 스레드가 일어나서 해당 메시지를 가져가 처리하도록 한다.
//------------------------------------
class MessageQueue
{
public:
	MessageQueue()
	{
		InitializeCriticalSection(&msg_cs);
	}
	~MessageQueue()
	{
		DeleteCriticalSection(&msg_cs);
	}

	int enqMsgbuf(char* c, int len)
	{
		int ret;
		EnterCriticalSection(&msg_cs);
		ret = MsgBuf.Enqueue(c, len);
		LeaveCriticalSection(&msg_cs);
		return ret;
	}

	int deqMsgbuf(char* c, int len)
	{
		int ret;
		EnterCriticalSection(&msg_cs);
		ret = MsgBuf.Dequeue(c, len);
		LeaveCriticalSection(&msg_cs);
		return ret;
	}
private:
	CRITICAL_SECTION msg_cs;
	CRingBuffer MsgBuf{ BUFSIZE + 1 };
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

	void DecreaseIOCount()
	{
		EnterCriticalSection(&session_cs);
		InterlockedDecrement((long*)&IOCount);
		LeaveCriticalSection(&session_cs);
	}

	CRITICAL_SECTION session_cs;
	SOCKET sock = INVALID_SOCKET;
	CRingBuffer recvBuf{ BUFSIZE + 1 };
	CRingBuffer sendBuf{ BUFSIZE + 1 };
	MessageQueue messageQueue;
	long long session_id = 0;
	LONG IsSending = 0;
	int IOCount = 0;
	IOCP_CONTEXT sendOverlapped{ ESend };
	IOCP_CONTEXT recvOverlapped{ ERecv };
	//IOCP_CONTEXT contentsOverlapped{ EContents };
};



//------------------------------------------
// 모든 IOCP의 핸들 정보를 가지고 있는 전역 싱글톤 객체
//------------------------------------------
class IOCPHandle
{
public:
	static IOCPHandle* GetIOCPHandleInstance()
	{
		if (IOCPHandleInstance == nullptr)
		{
			IOCPHandleInstance = new IOCPHandle;
		}
		return IOCPHandleInstance;
	}

	HANDLE netHcp = {};
	HANDLE contentHcp = {};
private:
	static IOCPHandle* IOCPHandleInstance;

	IOCPHandle() {};
	~IOCPHandle() {};

};
IOCPHandle* IOCPHandle::IOCPHandleInstance = nullptr;

//-------------------------------------------------------
// Accept 소켓 개수와 Delete 소켓 개수 비교
//-------------------------------------------------------
int g_acceptSockNum;
int g_deleteSockNum;

//--------------------------------
//세션과 세션 ID를 저장하기 위한 맵 
// 싱글톤으로 만들어서, 세션 포인터 반환받는 작업에 락걸고 동기화
//--------------------------------
class cSessionMap {
public:

	static cSessionMap* GetSessionMap()
	{
		if (sessionMapInstance == nullptr)
		{
			sessionMapInstance = new cSessionMap;
			atexit(Destroy);
		}
		return sessionMapInstance;
	}

	static void Destroy()
	{
		delete sessionMapInstance;
		sessionMapInstance = nullptr;
	}

	void AddSession(SOCKETINFO* psession)
	{
		EnterCriticalSection(&_sessionMap_cs);
		_sessionMap[_sessionCounter++] = psession;
		LeaveCriticalSection(&_sessionMap_cs);
	}

	void deleteSession(SOCKETINFO* &psession, char* s_ip, int i_port)
	{
		int id = psession->session_id;
		EnterCriticalSection(&_sessionMap_cs);
		_sessionMap[id] = nullptr;

		//누군가 세션 사용중인지 확인
		psession->GetSessionLock();
		psession->UnLockSession();
		closesocket(psession->sock);
		printf("[Network] 클라이언트 종료: IP 주소 = %s, 포트번호 = %d\n", s_ip, i_port);
		delete psession;
		psession = nullptr;

		LeaveCriticalSection(&_sessionMap_cs);
	}

	void GetSessionptr(long long sessionId, SOCKETINFO* &sessionptr)
	{
		EnterCriticalSection(&_sessionMap_cs);
		sessionptr = _sessionMap[sessionId];
		if (sessionptr != nullptr)
		{
			sessionptr->GetSessionLock();
		}
		LeaveCriticalSection(&_sessionMap_cs);
	}

	long long GetSessionCount()
	{
		return _sessionCounter;
	}

	void GetMapLock()
	{
		EnterCriticalSection(&_sessionMap_cs);
	}

	void UnLockMap()
	{
		LeaveCriticalSection(&_sessionMap_cs);
	}

private:
	//싱글톤 인스턴스
	static cSessionMap* sessionMapInstance;

	cSessionMap()
	{
		InitializeCriticalSection(&_sessionMap_cs);
	}
	~cSessionMap()
	{
		DeleteCriticalSection(&_sessionMap_cs);
	}

	//--------------------------------
	// 세션과 세션 ID를 저장하기 위한 맵 
	// 자료구조 : 배열
	// 최대치 : 8byte 크기
	//--------------------------------
	SOCKETINFO* _sessionMap[1 << 24] = {};
	long long _sessionCounter = 0;
	CRITICAL_SECTION _sessionMap_cs;
};
cSessionMap* cSessionMap::sessionMapInstance = nullptr;

//-------------------------------------------------------------
// 
//-------------------------------------------------------------


//작업자 스레드 함수
unsigned int __stdcall WorkerThread(LPVOID arg);

//컨텐츠 스레드 함수
unsigned int __stdcall ContentsThread(LPVOID arg);

//-----------------------------------------
// 세션 종료
// IO가 끝난 세션에 대해 완전히 삭제
//-----------------------------------------
void ReleaseSession(SOCKADDR_IN& clientaddr, SOCKETINFO*& ptr);

//------------------------------------------
// 세션 수신
// 세션 수신 링버퍼 상태 확인 후 WSAbuf에 등록, 해당 소켓에 대해 WSARecv 호출
//------------------------------------------
bool WsaRecvSession(SOCKADDR_IN& clientaddr, SOCKETINFO*& ptr);

//------------------------------------------
// 세션 송신
// 세션 송신 링버퍼 상태 확인 후 WSAbuf에 등록, 해당 소켓에 대해 WSASend 호출
//------------------------------------------
bool WsaSendSession(SOCKADDR_IN& clientaddr, SOCKETINFO*& ptr);

//---------------------------------------------
// 컨텐츠 수신
// 컨텐츠에게 세션에 대한 정보를 감추기 위해 전역 함수로 선언
// 세션의 메시지 큐에서 처리할 메시지를 꺼내어 컨텐츠에 전달한다.
//---------------------------------------------
int GetPacket(long long sessionId, char* msg, int len)
{
	SOCKETINFO* ptr;
	cSessionMap* pSessionMap = cSessionMap::GetSessionMap();
	pSessionMap->GetSessionptr(sessionId, ptr);
	if (ptr == nullptr)
	{
		return -1;
	}
	int ret = ptr->messageQueue.deqMsgbuf(msg, len);
	ptr->UnLockSession();
	return ret;
}

//----------------------------------------------
// 컨텐츠 송신
// 컨텐츠에게 세션에 대한 정보를 감추기 위해 전역 함수로 선언
// 세션의 송신 링버퍼에 메시지 담고, 네트워크 스레드에게 알려주는 작업까지 완료하자.
//----------------------------------------------
int SendPacket(long long sessionId, char* msg, int len)
{
	SOCKETINFO* ptr;
	cSessionMap* pSessionMap = cSessionMap::GetSessionMap();
	//PQCS로 워커 스레드를 깨우기 위해서 사용
	//IOCPHandle* pIOCPHandle = IOCPHandle::GetIOCPHandleInstance();
	pSessionMap->GetSessionptr(sessionId, ptr);
	if (ptr == nullptr)
	{
		//printf("[SendPacket] 이미 삭제가 발생한 세션에 대해 전송 시도\n");
		//이는 연결이 끊어지 세션에게 작업중이던 메시지를 더 이상 보내지 않음을 의미한다.
		//즉, 의도적인 데이터 유실임.
		return -1;
	}
	int ret = ptr->sendBuf.Enqueue(msg, len);
	//여기에서 Session의 Send를 발생시켜야 Session이 삭제되지 않는다.
			//클라이언트 정보 얻기
	SOCKADDR_IN clientaddr;
	int addrlen = sizeof(clientaddr);
	getpeername(ptr->sock, (SOCKADDR*)&clientaddr, &addrlen);
	//송신 링버퍼에 남은 데이터를 Send
	if (!WsaSendSession(clientaddr, ptr))
	{
		//안에서 세션 삭제가 일어난 경우 바로 GQCS 대기 루틴
		return -1;
	}
	//PostQueuedCompletionStatus(pIOCPHandle->netHcp, len, (ULONG_PTR)ptr, (LPWSAOVERLAPPED)&ptr->contentsOverlapped);
	ptr->UnLockSession();
	return ret;
}

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
	IOCPHandle* pIOCPHandle = IOCPHandle::GetIOCPHandleInstance();
	pIOCPHandle->netHcp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (pIOCPHandle->netHcp == NULL) return 1;
	pIOCPHandle->contentHcp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (pIOCPHandle->contentHcp == NULL) return 1;

	//CPU 개수 확인
	SYSTEM_INFO si;
	GetSystemInfo(&si);

	//(cpu 개수 * 2)개의 네트워크 작업자 스레드 생성
	HANDLE hThread;
	unsigned int uiThreadID; 

	for (int i = 0; i < (int)si.dwNumberOfProcessors * 2; i++)
	//for (int i = 0; i < 1; i++)
	{
		hThread = (HANDLE)_beginthreadex(
			NULL,           // Security attributes (NULL = 디폴트)
			0,              // Stack size (0 = 디폴트)
			WorkerThread,   // Thread function
			pIOCPHandle,    // Argument list to be passed to thread function
			0,              // Initial state (0 = 즉시 실행)
			&uiThreadID     // Pointer to thread ID
		);

		if (hThread == NULL) return 1;

		CloseHandle(hThread);
	}

	//컨텐츠 스레드 생성
	//for (int i = 0; i < (int)si.dwNumberOfProcessors * 2; i++)
	for (int i = 0; i < 1; i++)
	{
		hThread = (HANDLE)_beginthreadex(NULL, 0, ContentsThread, pIOCPHandle, 0, &uiThreadID);
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
	
	cSessionMap* sessionMap = cSessionMap::GetSessionMap();


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
		g_acceptSockNum++;
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
		long long id = sessionMap->GetSessionCount();
		ptr->session_id = id;
		sessionMap->AddSession(ptr);

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
					continue;
				}
			}
		}
	}



	//윈속 종료
	WSACleanup();
	return 0;
}



//작업자 스레드 함수
unsigned int __stdcall WorkerThread(LPVOID arg)
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

		//삭제된 세션에 대한 완료 통지가 온다면 무시하도록 한다.
		if (ptr == nullptr) continue;

		//클라이언트 정보 얻기
		SOCKADDR_IN clientaddr;
		int addrlen = sizeof(clientaddr);
		getpeername(ptr->sock, (SOCKADDR*)&clientaddr, &addrlen);

		//비동기 입출력 결과 확인
		if (cbTransferred == 0)
		{
			//클라가 종료신호 FIN보냄.
			printf("[Network] 클라이언트 종료 신호 수신: IP 주소 = %s, 포트번호 = %d\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
			InterlockedDecrement((long*)&d_gqcs_client);
			int decrease = InterlockedDecrement((long*)&ptr->IOCount);
			if (decrease == 0)
			{
				//세션에 대해 락을 얻는다  = 현재 sendpacket을 진행 중인지 확인
				//IsSending 확인 = send 중인지 확인, send 중이라면 이미 IO가 하나 커졌을때니깐, 0인지만 확인하면 되지 않을까?
				//락 얻고, 0인지 비교한 후 들어와서 락 푼다면?
					ReleaseSession(clientaddr, ptr);
			}
			else if (decrease < 0)
			{
				while (1)
				{
					printf("뭐야 이거\n");
				}
			}
			else
			{
				printf("[Network] 아직 IO 덜 끝남.\n");
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
				printf("[Network] ");
				err_display("WSAGetOverlappedResult()");
				InterlockedDecrement((long*)&d_gqcs);
				if (InterlockedDecrement((long*)&ptr->IOCount) == 0)
				{
					ReleaseSession(clientaddr, ptr);
					continue;
				}
			}
			else
			{
				//IO 실패
				printf("[Network] IO 실패\n");
				InterlockedDecrement((long*)&d_gqcs_IO);
				if (InterlockedDecrement((long*)&ptr->IOCount) == 0)
				{

						ReleaseSession(clientaddr, ptr);

					continue;
				}

			}

			continue;
		}

		if (lpOverlapped->op == ERecv)
		{
			//printf("[Network] 클라이언트 수신, 포트번호 = %d\n", ntohs(clientaddr.sin_port));
			

				if (ptr->recvBuf.MoveRear(cbTransferred) != 0)
				{
					//Recv 버퍼에 있는 내용 읽어서, send링버퍼에 담기
					Msg recvMsg;

					//이제 메시지 길이 단위로 읽어서 컨텐츠 스레드에 넘겨야 한다.
					//그래야 메시지 순서가 보장됨.
					//수신 링버퍼에 있는 데이터 중 헤더 길이만큼 있는 메시지는 모두 읽어서 처리
					//------------------------------------------------------
					// 1. 링 버퍼에서 Dequeue
					// 먼저 메시지 길이만큼 읽을 후, 해당 메시지 길이를 Dequeue
					//------------------------------------------------------
					while (1)
					{
						short Header_size = ptr->recvBuf.Peek((char*)&recvMsg.header, sizeof(recvMsg.header));
						if (Header_size == sizeof(recvMsg.header) && recvMsg.header != 0)
						{
							if (ptr->recvBuf.GetUseSize() >= recvMsg.header + sizeof(recvMsg.header))
							{
								//이미 읽은 헤더는 제외하고 읽게 만들자.
								ptr->recvBuf.MoveFront(sizeof(recvMsg.header));

								int dequeued_size = ptr->recvBuf.Dequeue((char*)&recvMsg.payload, recvMsg.header);
								if (dequeued_size != recvMsg.header)
								{
									while (1)
									{
										printf("[Network] 링버퍼 Dequeue 오류: 요청 %d, 실제 %d\n", recvMsg.header, dequeued_size);
									}
								}

								//메시지 버퍼에 추출한 메시지를 넣기.
								int ret = ptr->messageQueue.enqMsgbuf((char*)&recvMsg.payload, recvMsg.header);

								if (ret == 0)
								{
									while (1)
									{
										printf("[Network] 메시지 버퍼 꽉 찼음\n");
									}
								}
								else if (ret != recvMsg.header)
								{
									while (1)
									{
										printf("[Network] 메시지 버퍼 Enqueue 오류: 요청 %d, 실제 %d\n", recvMsg.header, ret);
									}
								}

								//여기서 PQCS 요청으로 컨텐츠 스레드에게 읽을 만큼 전달
								//이제 받은 메시지를 그대로 컨텐츠 스레드로 넘겨주는 것으로 네트워크 스레드의 할 일은 끝
								if (!PostQueuedCompletionStatus(iocpHandle->contentHcp, recvMsg.header, ptr->session_id, (LPWSAOVERLAPPED)lpOverlapped))
								{
									while (1)
									{
										printf("[Network] 컨텐츠 IOCP에 PQCS실패!\n");
										//---------이때 뭐하지?
										//---------1. 메시지 재전송
										//---------2. 그냥 뻑 내고 죽기
									}
								}
								else
								{
									//printf("[Network] PQCS호출 포트번호 = %d\n", ntohs(clientaddr.sin_port));
								}
								//남은 메시지 Dequeue는 컨텐츠 스레드가 할거임.
							}
							else
							{
								//수신 링버퍼가 가득차서 더이상 데이터를 받을 수 없는 상황
								//printf("[Network] payload가 아직 다 안 들어왔음\n");
								break;
							}
						}
						else
						{
							//printf("[Network] 헤더가 아직 다 안 들어왔음\n");
							break;
						}
					}
					
				}
				else
				{
					//수신 링버퍼가 가득차서 더이상 데이터를 받을 수 없는 상황
					//printf("[Network] 수신 링버퍼 꽉 찼음.\n");
				}
			
			

			//현재 스레드를 다시 Recv 등록하기
			//printf("[Network] 다시 recv 대기하기, 포트번호 = %d\n", ntohs(clientaddr.sin_port));
			if (!WsaRecvSession(clientaddr, ptr))
			{
				//안에서 세션 삭제가 일어난 경우 바로 GQCS 대기 루틴
				continue;
			}

			//GQCS Recv 완료통지에 대한 IO 감소
			InterlockedDecrement((long*)&d_recv);
			if (InterlockedDecrement((long*)&ptr->IOCount) == 0)
			{
				ReleaseSession(clientaddr, ptr);
				continue;
			}
		}
		//현재는 사용하지 않는다.
		// 나중에 SendPacket이나 GetPacket에서 링버퍼에 직접 접근하게 만들경우 사용해본다. 
		//컨텐츠 스레드로부터 완료 통지를 받은 경우
		//else if (lpOverlapped->op == EContents)
		//{
		//	//송신 링버퍼에 남은 데이터를 Send
		//	if (!WsaSendSession(clientaddr, ptr))
		//	{
		//		//안에서 세션 삭제가 일어난 경우 바로 GQCS 대기 루틴
		//		continue;
		//	}

		//}
		else if(lpOverlapped->op == ESend)
		{
			//락 풀기전에 Send한 크기만큼 송신 버퍼에서 movefront
			//ptr->sendBuf.GetLockBuffer();
			//ptr->GetSessionLock();
			ptr->sendBuf.MoveFront(cbTransferred);
			//ptr->UnLockSession();
			//ptr->sendBuf.UnLockBuffer();
			//송신 완료, 송신 플래그 해제
			if (InterlockedCompareExchange(&ptr->IsSending, 0, 1) == 0)
			{
				while (1)
				{
					printf("Send 중첩 발생, 포트번호 = %d\n", ntohs(clientaddr.sin_port));
				}
			}

			ptr->GetSessionLock();
			//송신 링버퍼에 남은 데이터를 Send
			if (!WsaSendSession(clientaddr, ptr))
			{
				//안에서 세션 삭제가 일어난 경우 바로 GQCS 대기 루틴
				continue;
			}
			ptr->UnLockSession();

			//GQCS Send 완료통지에 대한 IO 감소
			InterlockedDecrement((long*)&d_send);
			if (InterlockedDecrement((long*)&ptr->IOCount) == 0)
			{
				ReleaseSession(clientaddr, ptr);
				continue;
			}
		}
		else
		{
			
			while (1)
			{
				printf("[Network] : lpOverlapped 메시지 타입이 말도 안되는게 나옴.\n");
			}
		}
	}

	return 0;
}


unsigned int __stdcall ContentsThread(LPVOID arg)
{
	int retval;
	IOCPHandle* iocpHandle = (IOCPHandle*)arg;
	HANDLE hcp = iocpHandle->contentHcp;

	while (1) {
		//비동기 입출력 완료 기다리기
		DWORD cbTransferred;
		long long SessionID;
		IOCP_CONTEXT* lpOverlapped;
		retval = GetQueuedCompletionStatus(hcp, &cbTransferred, (PULONG_PTR)&SessionID, (LPOVERLAPPED*)&lpOverlapped, INFINITE);

		//비동기 입출력 결과 확인
		if (cbTransferred == 0)
		{
			//서버가 0짜리 던져줌.
			while (1)
			{
				printf("[Contents] 서버쉨 나한테 0 던짐.\n");
			}
		}
		else if (retval == 0)
		{
			while (1)
			{
				printf("[Contents] 비동기 함수를 호출하지 않고는 나올 수 없는 경우\n");
			}
		}


		//네트워크에서 넘긴 길이만큼 추출
		//Msg recvMsg; 굳이 동적할당 해야할까? 어차피 송신 링버퍼에 복사가 되었다면 문제 없는게 정상임.
		Msg* recvMsg  = new Msg;

		int ret = GetPacket(SessionID,recvMsg->payload, cbTransferred);
		if (ret == 0)
		{
			while (1)
			{
				printf("[Contents] 메시지 버퍼에 남은 메시지 없는데 추출 시도함.\n");
			}
		}
		else if (ret == -1)
		{
			//printf("[Contents] 세션이 이미 삭제됨.\n");
			delete recvMsg;
			//send까지 진행할 필요없음. 이미 삭제됨
			continue;
		}
		else if (ret != cbTransferred)
		{
			while (1)
			{
				printf("[Contents] 메시지 버퍼 추출 길이가 다름, 요청  : %d , 실제 : %d \n", cbTransferred, ret);
			}
		}

		//printf("[Contents] : 수신 메시지 내용 %lld\n", (long long)recvMsg->payload);

		//추출한 메시지에 헤더를 붙여서 SendPakcet
		recvMsg->header = sizeof(recvMsg->payload);
		int sendret = SendPacket(SessionID, (char*)recvMsg, sizeof(Msg));
		if (sendret == 0)
		{
			while (1)
			{
				printf("[Contents] 네트워크 송신 버퍼가 꽉 참\n");
			}
		}
		else if (sendret == -1)
		{
			//printf("[Contents] 세션이 이미 삭제됨.\n");
		}
		else if(sendret != sizeof(Msg))
		{
			while (1)
			{
				printf("[Contents] 메시지 버퍼 추출 길이와 송신 버퍼에 넣은 길이가 다름\n");
			}
		}
		
		delete recvMsg;
	}

	return 0;
}

void ReleaseSession(SOCKADDR_IN& clientaddr, SOCKETINFO*& ptr)
{
	cSessionMap* psm = cSessionMap::GetSessionMap();
	
	
	psm->deleteSession(ptr, inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

	InterlockedIncrement((long*)&g_deleteSockNum);
	if (InterlockedCompareExchange((long*)&g_deleteSockNum,g_acceptSockNum, g_acceptSockNum) == g_acceptSockNum)
	{
		printf("[Network] Accept 횟수와 Delete횟수가 일치했음.");
		_CrtDumpMemoryLeaks();
	}
}

bool WsaRecvSession(SOCKADDR_IN& clientaddr, SOCKETINFO*& ptr)
{
	int retval;
	
	ZeroMemory(&ptr->recvOverlapped, sizeof(ptr->recvOverlapped));
	ptr->recvOverlapped.op = ERecv;

	cSessionMap* sessionMap = cSessionMap::GetSessionMap();

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
				printf("[Network] ");
				err_display("WSARecv()");
				InterlockedDecrement((long*)&d_recv);
				if (InterlockedDecrement((long*)&ptr->IOCount) == 0)
				{
					ReleaseSession(clientaddr, ptr);
					return false;
				}
			}
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
				printf("[Network] ");
				err_display("WSARecv()");
				InterlockedDecrement((long*)&d_recv);
				if (InterlockedDecrement((long*)&ptr->IOCount) == 0)
				{
					ReleaseSession(clientaddr, ptr);
					return false;
				}
			}
		}
	}
	
	return true;
}

bool WsaSendSession(SOCKADDR_IN& clientaddr, SOCKETINFO*& ptr)
{

	if (ptr == nullptr)
	{
		while (1)
		{
			printf("[Network] 송신 시도중인 세션이 이미 삭제된 세션\n");
		}
	}




	int retval;
	//Send 중이 아니라면
	//Send 링버퍼에 있는 있는 내용 전부 Send
	if (InterlockedCompareExchange(&ptr->IsSending, 1, 0) == 0)
	{
		//이미 한발 앞서서 처리된 경우
		//다시 해제 시켜준다.
		//ptr->sendBuf.GetLockBuffer();
		if (ptr->sendBuf.GetUseSize() == 0)
		{
			
			if (InterlockedCompareExchange(&ptr->IsSending, 0, 1) == 1)
			{
				//ptr->sendBuf.UnLockBuffer();
				return true;
			}
			else
			{
				while (1)
				{
					printf("[Network] 그새 중첩이 발생했다고??\n");
				}
			}
			
		}
		ptr->sendBuf.UnLockBuffer();

		//printf("[Network] 송신 진행 중 아님, 송신 루트 탐., 포트번호 = %d\n", ntohs(clientaddr.sin_port));
		ZeroMemory(&ptr->sendOverlapped, sizeof(ptr->sendOverlapped));
		ptr->sendOverlapped.op = ESend;

		int sendlen = ptr->sendBuf.GetUseSize();
		if (sendlen == 0)
		{
			while (1)
			{
				printf("[Network] 미친 지금 0짜리 보낼뻔\n");
			}
		}
		if (sendlen > ptr->sendBuf.DirectDequeueSize())
		{
			WSABUF wsabuf[2];
			wsabuf[0].buf = ptr->sendBuf.GetFrontBufferPtr();
			wsabuf[0].len = ptr->sendBuf.DirectDequeueSize();
			int frontSize = ptr->sendBuf.GetBufferSize() - ptr->sendBuf.GetFreeSize() - ptr->sendBuf.DirectDequeueSize();
			wsabuf[1].buf = ptr->sendBuf.GetRearBufferPtr() - frontSize;
			wsabuf[1].len = frontSize;
			

			int increase = InterlockedIncrement((long*)&ptr->IOCount);
			if (increase == 1)
			{
				printf("[Network] 누군가 정리 중인 것으로 보임. 송신 진행 불가. 포트번호 = %d\n", ntohs(clientaddr.sin_port));
				ptr->UnLockSession();
				//누군가가 정리중이라면 send를 진행시키지 않는다.
				return false;
				/*while (1)
				{
					printf("[Network] send완료 시 감소 전이므로 발생해선 안되며, sendPacket에서도 삭제되었음변 아예 송신 못함.\n");
				}*/
			}
			else
			{
				InterlockedIncrement((long*)&i_send);
			}
			//printf("[Network] 데이터 송신  포트번호 = %d\n", ntohs(clientaddr.sin_port));
			retval = WSASend(ptr->sock, wsabuf, 2, (LPDWORD)&sendlen, 0, (LPWSAOVERLAPPED)&ptr->sendOverlapped, NULL);


			if (retval == SOCKET_ERROR)
			{
				if (WSAGetLastError() != WSA_IO_PENDING)
				{
					printf("[Network] ");
					err_display("WSASend()");
					InterlockedDecrement((long*)&d_send);
					if (InterlockedDecrement((long*)&ptr->IOCount) == 0)
					{
						//InterlockedCompareExchange(&ptr->IsSending, 0, 1);
						ReleaseSession(clientaddr, ptr);
						return false;
					}
				}
			}
		}
		else
		{
			WSABUF wsabuf;
			wsabuf.buf = ptr->sendBuf.GetFrontBufferPtr();
			wsabuf.len = ptr->sendBuf.DirectDequeueSize();

			int increase = InterlockedIncrement((long*)&ptr->IOCount);
			if (increase == 1)
			{
				printf("[Network] 누군가 정리 중인 것으로 보임. 송신 진행 불가. 포트번호 = %d\n", ntohs(clientaddr.sin_port));
				ptr->UnLockSession();
				return false;
				/*while (1)
				{
					printf("[Network] send완료 시 감소 전이므로 발생해선 안되며, sendPacket에서도 삭제되었음변 아예 송신 못함.\n");
				}*/
			}
			else
			{
				InterlockedIncrement((long*)&i_send);
			}
			//printf("[Network] 데이터 송신  포트번호 = %d\n", ntohs(clientaddr.sin_port));
			retval = WSASend(ptr->sock, &wsabuf, 1, (LPDWORD)&sendlen, 0, (LPWSAOVERLAPPED)&ptr->sendOverlapped, NULL);


			if (retval == SOCKET_ERROR)
			{
				if (WSAGetLastError() != WSA_IO_PENDING)
				{
					printf("[Network] ");
					err_display("WSASend()");
					InterlockedDecrement((long*)&d_send);
					if (InterlockedDecrement((long*)&ptr->IOCount) == 0)
					{
						ReleaseSession(clientaddr, ptr);
						return false;
					}
				}
			}
		}
	}
	else
	{
		//printf("[Network] 이미 송신 진행 중  포트번호 = %d\n", ntohs(clientaddr.sin_port));
	}


	return true;
}

