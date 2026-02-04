#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma comment(lib, "ws2_32")

#include "CLanServer.h"
#include "CIOCPHandle.h"
#include "cSessionMap.h"
#include "Session.h"
#include "CRingBuffer.h"
#include  "CPacketRingBuffer.h"
#include "OVERLAPPED_CONTEXT.h"
#include "MessageQueue.h"
#include "errlog.h"
#include "CPacketForMultiThread.h"

#define SERVERPORT (6000)
#define BUFSIZE (1024 * 16)
#define MSG_SIZE (8)
#define WSASEND_MAX_BUFFER_COUNT (128)

//------------------------------------
//메시지 프로토콜
// 헤더 2Byte (길이)
// 데이터 8Byte(에코)
//------------------------------------
struct Msg {
	short header = 0;
	char payload[MSG_SIZE] = {};
};



bool CLanServer::Start()
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
		if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return false;

		//네트워크, 컨텐츠 스레드 입출력 완료 포트 생성
		IOCPHandle* pIOCPHandle = IOCPHandle::GetIOCPHandleInstance();
		pIOCPHandle->netHcp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
		if (pIOCPHandle->netHcp == NULL) return false;
		pIOCPHandle->contentHcp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
		if (pIOCPHandle->contentHcp == NULL) return false;

		//CPU 개수 확인
		SYSTEM_INFO si;
		GetSystemInfo(&si);

		//(cpu 개수 * 2)개의 네트워크 작업자 스레드 생성
		HANDLE hThread;
		unsigned int uiThreadID;

		ServerAndHandle* sah = new ServerAndHandle;
		sah->phandle = pIOCPHandle;
		sah->thisptr = this;

		for (int i = 0; i < (int)si.dwNumberOfProcessors * 2; i++)
			//for (int i = 0; i < 1; i++)
		{
			hThread = (HANDLE)_beginthreadex(
				NULL,           // Security attributes (NULL = 디폴트)
				0,              // Stack size (0 = 디폴트)
				WorkerThread,   // Thread function
				sah,    // Argument list to be passed to thread function
				0,              // Initial state (0 = 즉시 실행)
				&uiThreadID     // Pointer to thread ID
			);

			if (hThread == NULL) return false;

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

			//RST를 보내기위한 소켓 옵션
			LINGER optval;
			optval.l_onoff = 1;
			optval.l_linger = 0;
			retval = setsockopt(client_sock, SOL_SOCKET, SO_LINGER, (char*)&optval, sizeof(optval));
			if (retval == SOCKET_ERROR) {
				err_quit("setsockopt()");
				break;
			}

			//클라이언트 IP 차단
			if (!OnConnectionRequest(inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port)))
			{
				closesocket(client_sock);
			}

			
			SOCKETINFO* ptr = sessionMap->MakeNewSession(client_sock);

			long long id = ptr->session_id;
			WSABUF wsabuf;
			wsabuf.buf = ptr->recvBuf->GetFrontBufferPtr();
			wsabuf.len = ptr->recvBuf->GetFreeSize();

			printf("[TCP 서버] 클라이언트 접속 : IP 주소 = %s, 포트번호 = %d\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
			OnClientJoin(clientaddr, id);

			//소켓과 입출력 완료 포트 연결
			CreateIoCompletionPort((HANDLE)client_sock, pIOCPHandle->netHcp, (ULONG_PTR)ptr, 0);

			//비동기 입출력 시작
			flags = 0;
			//IOCount를 증가시켰는데 1이라면, 정리중인 세션이므로
			//더 이상 송수신 처리가 일어나지 않도록 한다.
			InterlockedIncrement((long*)&ptr->IOCount);
			retval = WSARecv(client_sock, &wsabuf, 1, &recvbytes, &flags, (LPWSAOVERLAPPED)ptr->recvOverlapped, NULL);
			if (retval == SOCKET_ERROR)
			{
				if (WSAGetLastError() != ERROR_IO_PENDING) {
					err_display("WSARECV()");

					//여기서 IOCount의 최상위 비트를 SessionReleaseFlag로 사용한다면??
					//IOCount가 0이면 비트에 1넣기

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
		return true;
}

void CLanServer::Stop()
{
}

int CLanServer::GetSessionCount()
{
    return _sessionCount;
}

bool CLanServer::Disconnect(SessionID sessionId)
{
	SOCKETINFO* ptr;
	cSessionMap* pSessionMap = cSessionMap::GetSessionMap();

	pSessionMap->GetSessionptr(sessionId, ptr);
	if (ptr == nullptr)
	{
		return false;
	}
	if (shutdown(ptr->_sock, SD_BOTH) != 0)
	{
		return false;
	}
    return true;
}

bool CLanServer::SendPacket(SessionID sessionId, CPacket* cp)
{
	SOCKETINFO* ptr;
	cSessionMap* pSessionMap = cSessionMap::GetSessionMap();

	//네트워크 헤더를 삽입한다.
	//헤더에 삽입할 크기
	short netHeaderData = cp->GetDataSize() - sizeof(Msg::header);
	//패킷의 네트워크 헤더 부에 삽입
	short* phearder = (short*)cp->GetBufferPtr();
	*phearder = netHeaderData;

	pSessionMap->GetSessionptr(sessionId, ptr);
	if (ptr == nullptr)
	{
		return false;
	}

	cp->AddRef();
	int ret = ptr->sendBuf->Enqueue(cp);
	if (ret == 0)
	{
		printf("[Network]  Enqueue 실패\n");
		__debugbreak();
	}

	//여기에서 Session의 Send를 발생시켜야 Session이 삭제되지 않는다.
	//클라이언트 정보 얻기
	SOCKADDR_IN clientaddr;
	int addrlen = sizeof(clientaddr);
	getpeername(ptr->_sock, (SOCKADDR*)&clientaddr, &addrlen);
	//송신 링버퍼에 남은 데이터를 Send
	if (!WsaSendSession(clientaddr, ptr))
	{
		//안에서 세션 삭제가 일어난 경우 바로 GQCS 대기 루틴
		return false;
	}
	//PostQueuedCompletionStatus(pIOCPHandle->netHcp, len, (ULONG_PTR)ptr, (LPWSAOVERLAPPED)&ptr->contentsOverlapped);
	//ptr->UnLockSession();
    return true;
}

int CLanServer::getAcceptTPS()
{
    return _acceptTPS;
}

int CLanServer::getRecvMessageTPS()
{
    return _recvMessageTPS;
}

int CLanServer::getSendMessageTPS()
{
    return _sendMessageTPS;
}


void CLanServer::ReleaseSession(SOCKADDR_IN& clientaddr, SOCKETINFO*& ptr)
{
	//long ReleaseFlag = 1 << 31;
	//long oldIOCount = ptr->IOCount | ~ReleaseFlag;
	////Flag를 제거한 IOCount가 0이 아니면 return
	//if (0 != oldIOCount) return;

	//long CountwithFlagBit = (ReleaseFlag & oldIOCount);
	//if (InterlockedCompareExchange((long*)&ptr->IOCount, CountwithFlagBit, oldIOCount) != oldIOCount)
	//{
	//	return;
	//}

	cSessionMap* psm = cSessionMap::GetSessionMap();


	psm->deleteSessionptrFromSessionMap(ptr, inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

	//InterlockedIncrement((long*)&g_deleteSockNum);
	//if (InterlockedCompareExchange((long*)&g_deleteSockNum, g_acceptSockNum, g_acceptSockNum) == g_acceptSockNum)
	//{
	//	printf("[Network] Accept 횟수와 Delete횟수가 일치했음.");
	//	_CrtDumpMemoryLeaks();
	//}
}

//작업자 스레드 함수
unsigned int __stdcall CLanServer::WorkerThread(LPVOID arg)
{
	int retval;


	ServerAndHandle* sah = (ServerAndHandle*)arg;

	CLanServer* pServer = sah->thisptr;
	IOCPHandle* iocpHandle = sah->phandle;

	//IOCPHandle* iocpHandle = (IOCPHandle*)arg;
	HANDLE hcp = iocpHandle->netHcp;


	while (1) {
		//비동기 입출력 완료 기다리기
		DWORD cbTransferred;
		SOCKET client_sock;
		SOCKETINFO* ptr;
		OVERLAPPED_CONTEXT* lpOverlapped;
		retval = GetQueuedCompletionStatus(hcp, &cbTransferred, (PULONG_PTR)&ptr, (LPOVERLAPPED*)&lpOverlapped, INFINITE);

		//삭제된 세션에 대한 완료 통지가 온다면 무시하도록 한다.
		if (ptr == nullptr) continue;

		//클라이언트 정보 얻기
		SOCKADDR_IN clientaddr;
		int addrlen = sizeof(clientaddr);
		getpeername(ptr->_sock, (SOCKADDR*)&clientaddr, &addrlen);

		//비동기 입출력 결과 확인
		if (cbTransferred == 0)
		{
			//클라가 종료신호 FIN보냄.
			//printf("[Network] 클라이언트 종료 신호 수신: IP 주소 = %s, 포트번호 = %d\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
			int decrease = InterlockedDecrement((long*)&ptr->IOCount);
			if (decrease == 0)
			{
				//세션에 대해 락을 얻는다  = 현재 sendpacket을 진행 중인지 확인
				//IsSending 확인 = send 중인지 확인, send 중이라면 이미 IO가 하나 커졌을때니깐, 0인지만 확인하면 되지 않을까?
				//락 얻고, 0인지 비교한 후 들어와서 락 푼다면?
				pServer->ReleaseSession(clientaddr, ptr);
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
				//printf("[Network] 아직 IO 덜 끝남.\n");
			}

			continue;
		}
		else if (retval == 0)
		{
			DWORD lpcbTransfer, temp2;
			bool isIOSuccess = WSAGetOverlappedResult(ptr->_sock, (LPWSAOVERLAPPED)&lpOverlapped, &lpcbTransfer, false, &temp2);
			if (isIOSuccess && lpcbTransfer > 0)
			{
				//GQCS 실패
				printf("[Network] ");
				err_display("WSAGetOverlappedResult()");
				if (InterlockedDecrement((long*)&ptr->IOCount) == 0)
				{
					pServer->ReleaseSession(clientaddr, ptr);
					continue;
				}
			}
			else
			{
				//IO 실패
				printf("[Network] IO 실패\n");
				if (InterlockedDecrement((long*)&ptr->IOCount) == 0)
				{

					pServer->ReleaseSession(clientaddr, ptr);

					continue;
				}

			}

			continue;
		}

		if (lpOverlapped->op == ERecv)
		{
			//printf("[Network] 클라이언트 수신, 포트번호 = %d\n", ntohs(clientaddr.sin_port));


			if (ptr->recvBuf->MoveRear(cbTransferred) != 0)
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
					short Header_size = ptr->recvBuf->Peek((char*)&recvMsg.header, sizeof(recvMsg.header));
					if (Header_size == sizeof(recvMsg.header) && recvMsg.header != 0)
					{
						if (ptr->recvBuf->GetUseSize() >= recvMsg.header + sizeof(recvMsg.header))
						{
							//이미 읽은 헤더는 제외하고 읽게 만들자.
							ptr->recvBuf->MoveFront(sizeof(recvMsg.header));

							int dequeued_size = ptr->recvBuf->Dequeue((char*)&recvMsg.payload, recvMsg.header);
							if (dequeued_size != recvMsg.header)
							{
								while (1)
								{
									printf("[Network] 링버퍼 Dequeue 오류: 요청 %d, 실제 %d\n", recvMsg.header, dequeued_size);
								}
							}

							//메시지 버퍼에 추출한 메시지를 넣기.

							CPacket* pContentsSendPacket = CPacket::Alloc();
							int ret = pContentsSendPacket->PutData((char*)&recvMsg.payload, recvMsg.header);

							if (ret == 0)
							{
								while (1)
								{
									printf("[Network] 직렬화 버퍼 삽입 결과가 0\n");
								}
							}
							else if (ret != recvMsg.header)
							{
								while (1)
								{
									printf("[Network] 직렬화 버퍼 삽입 오류: 요청 %d, 실제 %d\n", recvMsg.header, ret);
								}
							}

							//컨텐츠의 수신 로직 실행
							pContentsSendPacket->AddRef();
							pServer->OnRecv(ptr->session_id, pContentsSendPacket);
							pContentsSendPacket->SubRef();
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
			if (!pServer->WsaRecvSession(clientaddr, ptr))
			{
				//안에서 세션 삭제가 일어난 경우 바로 GQCS 대기 루틴
				continue;
			}

			//GQCS Recv 완료통지에 대한 IO 감소
			if (InterlockedDecrement((long*)&ptr->IOCount) == 0)
			{
				pServer->ReleaseSession(clientaddr, ptr);
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
		else if (lpOverlapped->op == ESend)
		{
			//락 풀기전에 Send한 크기만큼 송신 버퍼에서 movefront
			//long l = 0;
			//InterlockedExchange(&l, 1);
			int sendPacketNum = ptr->sendPacketNum;
			int srfront = ptr->sendBuf->GetFront();
			int cpysrfront = srfront;
			int srCapacity = ptr->sendBuf->GetBufferSize();
			CPacket** ppacket = ptr->sendBuf->GetBufPtr();

			//패킷 미리 해제
			for (int i = 0; i < sendPacketNum; i++)
			{
				CPacket* packet = ppacket[cpysrfront];
				packet->SubRef();
				cpysrfront = (cpysrfront + 1) % srCapacity;
			}

			//ptr->GetSessionLock();
			ptr->sendBuf->MoveFront(sendPacketNum);
			//ptr->UnLockSession();
			
			//송신 완료, 송신 플래그 해제
			if (InterlockedCompareExchange(&ptr->IsSending, 0, 1) == 0)
			{
				while (1)
				{
					printf("Send 중첩 발생, 포트번호 = %d\n", ntohs(clientaddr.sin_port));
				}
			}

			//ptr->GetSessionLock();
			//송신 링버퍼에 남은 데이터를 Send
			//ptr->sendBuf->GetLockBuffer();
			if (!pServer->WsaSendSession(clientaddr, ptr))
			{
				//안에서 세션 삭제가 일어난 경우 바로 GQCS 대기 루틴
				//ptr->sendBuf->UnLockBuffer();
				continue;
			}
			//ptr->sendBuf->UnLockBuffer();
			//ptr->UnLockSession();

			//GQCS Send 완료통지에 대한 IO 감소
			if (InterlockedDecrement((long*)&ptr->IOCount) == 0)
			{
				pServer->ReleaseSession(clientaddr, ptr);
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


bool CLanServer::WsaRecvSession(SOCKADDR_IN& clientaddr, SOCKETINFO*& ptr)
{
	int retval;

	ptr->recvOverlapped->op = ERecv;
	ZeroMemory(ptr->recvOverlapped, sizeof(OVERLAPPED));
	

	cSessionMap* sessionMap = cSessionMap::GetSessionMap();

	//Direct로 넣을 수 있냐 없냐에 따라 여러 버퍼로 나눠서 받아야 함.
	int recvlen = ptr->recvBuf->GetFreeSize();
	if (recvlen > ptr->recvBuf->DirectEnqueueSize())
	{
		WSABUF wsabuf[2];
		wsabuf[0].buf = ptr->recvBuf->GetRearBufferPtr();
		wsabuf[0].len = ptr->recvBuf->DirectEnqueueSize();
		int frontSize = ptr->recvBuf->GetFreeSize() - ptr->recvBuf->DirectEnqueueSize();
		wsabuf[1].buf = ptr->recvBuf->GetFrontBufferPtr() - frontSize;
		wsabuf[1].len = frontSize;
		DWORD recvbytes;
		DWORD flags = 0;
		InterlockedIncrement((long*)&ptr->IOCount);
		retval = WSARecv(ptr->_sock, wsabuf, 2, &recvbytes, &flags, (LPWSAOVERLAPPED)ptr->recvOverlapped, NULL);

		if (retval == SOCKET_ERROR)
		{
			if (WSAGetLastError() != WSA_IO_PENDING)
			{
				//printf("[Network] ");
				//err_display("WSARecv()");
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
		wsabuf.buf = ptr->recvBuf->GetRearBufferPtr();
		wsabuf.len = ptr->recvBuf->DirectEnqueueSize();
		DWORD recvbytes;
		DWORD flags = 0;
		InterlockedIncrement((long*)&ptr->IOCount);
		retval = WSARecv(ptr->_sock, &wsabuf, 1, &recvbytes, &flags, (LPWSAOVERLAPPED)ptr->recvOverlapped, NULL);

		if (retval == SOCKET_ERROR)
		{
			if (WSAGetLastError() != WSA_IO_PENDING)
			{
				//printf("[Network] ");
				//err_display("WSARecv()");
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

bool CLanServer::WsaSendSession(SOCKADDR_IN& clientaddr, SOCKETINFO*& ptr)
{
	if (ptr == nullptr)
	{
		printf("[WsaSendSession] 송신 시도중인 세션이 이미 삭제된 세션\n");
		__debugbreak();
	}
	CPacketRingBuffer* prb = ptr->sendBuf;
	int retval;
	//Send 중이 아니라면
	//Send 링버퍼에 있는 있는 내용 전부 Send

	//세션에 락을 걸기 전에 올바른 송신 진행을 위해 작업한 내용이었는데
	//세션에 락을 걸고 송신을 진행한다는게 보장된다면 없어도 되지 않나? 테스트 필요
	if (InterlockedCompareExchange(&ptr->IsSending, 1, 0) == 0)
	{
		//이미 한발 앞서서 처리된 경우
		//다시 해제 시켜준다.
		//ptr->sendBuf.GetLockBuffer();
		if (ptr->sendBuf->GetUseSize() == 0)
		{
			//__debugbreak();
			//다른 워커 스레드가 수신을 완료한 이후 한번더 send를 하면서 링버퍼에 남아있는 처리까지 완료한 경우
			//이렇게 되면 이미 처리가 된 것이므로 return true하면 됨.
			if (InterlockedCompareExchange(&ptr->IsSending, 0, 1) == 1)
			{
				//ptr->sendBuf.UnLockBuffer();
				//__debugbreak();
				return true;
			}
			else
			{
				printf("[WsaSendSession] 그새 중첩이 발생했다고??\n");
				__debugbreak();
			}

		}
		//ptr->sendBuf->UnLockBuffer();

		//printf("[Network] 송신 진행 중 아님, 송신 루트 탐., 포트번호 = %d\n", ntohs(clientaddr.sin_port));
		ptr->sendOverlapped->op = ESend;
		ZeroMemory(ptr->sendOverlapped, sizeof(OVERLAPPED));

		int remain = prb->GetUseSize();

		if (remain == 0)
		{
			printf("[WsaSendSession] 미친 지금 0짜리 보낼뻔\n");
			__debugbreak();
		}

		WSABUF wsabuf[WSASEND_MAX_BUFFER_COUNT] = {0,};

		int bufIndex = 0;
		int rbFront = prb->GetFront();
		int rbCpacity = prb->GetBufferSize();
		CPacket** cpacket = prb->GetBufPtr();

		if (remain > WSASEND_MAX_BUFFER_COUNT)
		{
			printf("[WsaSendSession] wsabuf 용량보다 sendlen가 더 큰 경우\n");
			__debugbreak();
		}

		for (int i = 0; i < remain; i++)
		{
			CPacket* frontpacket = cpacket[rbFront];

			wsabuf[bufIndex].buf = frontpacket->GetBufferPtr();
			wsabuf[bufIndex].len = frontpacket->GetDataSize();

			bufIndex++;
			rbFront = (rbFront + 1) % rbCpacity;
		}

		int increase = InterlockedIncrement((long*)&ptr->IOCount);
		if (increase == 1)
		{
			printf("[Network] 누군가 정리 중인 것으로 보임. 송신 진행 불가. 포트번호 = %d\n", ntohs(clientaddr.sin_port));
			//ptr->UnLockSession();
			return false;
			/*while (1)
			{
				printf("[Network] send완료 시 감소 전이므로 발생해선 안되며, sendPacket에서도 삭제되었음변 아예 송신 못함.\n");
			}*/
		}
		else
		{
			/*InterlockedIncrement((long*)&i_send);*/
		}

		if (wsabuf[0].len == 0)
		{
			printf("[WsaSendSession] wsabuf 에 아무 값도 안들어갔음.\n");
			__debugbreak();
		}

		DWORD sendBytes = 0;
		//printf("[Network] 데이터 송신  포트번호 = %d\n", ntohs(clientaddr.sin_port));

		ptr->sendPacketNum = remain;
		retval = WSASend(ptr->_sock, wsabuf, remain, (LPDWORD)&sendBytes, 0, (LPWSAOVERLAPPED)ptr->sendOverlapped, NULL);

		if (retval == SOCKET_ERROR)
		{
			if (WSAGetLastError() != WSA_IO_PENDING)
			{
				//보낼 세션이 접속을 끊어버린 상황
				//__debugbreak();
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
		//printf("[Network] 이미 송신 진행 중  포트번호 = %d\n", ntohs(clientaddr.sin_port));
		//이때 락걸고 대기해야 하는거아닌가?
	}


	return true;
}

