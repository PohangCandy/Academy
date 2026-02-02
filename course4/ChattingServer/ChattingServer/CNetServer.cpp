#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma comment(lib, "ws2_32")

#include "CNetServer.h"
#include "CIOCPHandle.h"
#include "cSessionMap.h"
#include "Session.h"
#include "CRingBuffer.h"
#include "OVERLAPPED_CONTEXT.h"
#include "MessageQueue.h"
#include "errlog.h"
#include "CPacketForMultiThread.h"

#define SERVERPORT (12001)
#define BUFSIZE (1024 * 1024)
#define MSG_SIZE (8)

//------------------------------------
//메시지 프로토콜
// 헤더 2Byte (길이)
// 데이터 8Byte(에코)
//------------------------------------
struct Msg {
	short header = 0;
	char payload[MSG_SIZE] = {};
};



bool CNetServer::Start()
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
		_hWorkerThreadIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
		if (_hWorkerThreadIOCP == NULL) return false;
		//pIOCPHandle->contentHcp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
		//if (pIOCPHandle->contentHcp == NULL) return false;

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
				NULL,          
				0,              
				WorkerThread,   
				this,   
				0,              
				&uiThreadID     
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

			//지금 접곡한 클라이언트에 대한 차단
			if (!OnConnectionRequest(inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port)))
			{
				closesocket(client_sock);
			}

			
			//소켓 정보 구조체 할당
			SOCKETINFO* ptr = new SOCKETINFO(BUFSIZE);

			if (ptr == NULL) break;
			//overlap의 op값을 살리기위해서 overlap 구조체 크기만크만 지운다.
			ZeroMemory(ptr->recvOverlapped, sizeof(OVERLAPPED));
			ptr->sock = client_sock;
			ptr->recvBuf->ClearBuffer();
			ptr->sendBuf->ClearBuffer();
			WSABUF wsabuf;
			wsabuf.buf = ptr->recvBuf->GetFrontBufferPtr();
			wsabuf.len = ptr->recvBuf->GetFreeSize();

			//세션을 맵에 저장
			InterlockedIncrement((long*)&_sessionCount);
			long long id = sessionMap->InsertSessionptrToSessionMap(ptr);
			ptr->session_id = id;
			

			//printf("[TCP 서버] 클라이언트 접속 : IP 주소 = %s, 포트번호 = %d\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
			OnClientJoin(clientaddr, id);

			//소켓과 입출력 완료 포트 연결
			CreateIoCompletionPort((HANDLE)client_sock, _hWorkerThreadIOCP, (ULONG_PTR)ptr, 0);

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

void CNetServer::Stop()
{
}

int CNetServer::GetSessionCount()
{
    return _sessionCount;
}

bool CNetServer::Disconnect(SessionID sessionId)
{
	SOCKETINFO* ptr;
	cSessionMap* pSessionMap = cSessionMap::GetSessionMap();

	pSessionMap->GetSessionptr(sessionId, ptr);
	if (ptr == nullptr)
	{
		return false;
	}
	if (shutdown(ptr->sock, SD_RECEIVE) != 0)
	{
		return false;
	}
	//ptr->DecreaseIOCount();
    return true;
}

bool CNetServer::SendPacket(SessionID sessionId, CPacket* cp)
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
	ptr->sendBuf->GetLockBuffer();
	cp->Encode(0xa9);
	int ret = ptr->sendBuf->Enqueue(cp->GetBufferPtr(), cp->GetDataSize());
	ptr->sendBuf->UnLockBuffer();
	if (ret == 0)
	{
		while (1)
		{
			printf("[Network] 네트워크 송신 버퍼가 다 참\n");
		}
	}
	else if (ret != cp->GetDataSize())
	{
		while (1)
		{
			printf("[Network]  송신 버퍼에 넣은 길이와 메시지 길이가 다름\n");
		}
	}
	//여기에서 Session의 Send를 발생시켜야 Session이 삭제되지 않는다.
	//클라이언트 정보 얻기
	SOCKADDR_IN clientaddr;
	int addrlen = sizeof(clientaddr);
	getpeername(ptr->sock, (SOCKADDR*)&clientaddr, &addrlen);
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

int CNetServer::getAcceptTPS()
{
    return _acceptTPS;
}

int CNetServer::getRecvMessageTPS()
{
    return _recvMessageTPS;
}

int CNetServer::getSendMessageTPS()
{
    return _sendMessageTPS;
}


void CNetServer::ReleaseSession(SOCKADDR_IN& clientaddr, SOCKETINFO*& ptr)
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

bool CNetServer::Decode(MsgHeader* pHeader, char* pc)
{
	unsigned char checksum = 0;
	unsigned char beforeparaP = 0;
	unsigned char afterparaP = 0;
	unsigned char beforeDecodeP = 0;
	unsigned char afterDecodeP = 0;

	int payLoadSize = pHeader->Len;
	for (int i = 0; i < payLoadSize; i++)
	{
		char* pPacketChar = &pc[sizeof(MsgHeader) + i];
		afterDecodeP = *pPacketChar;

		afterparaP = *pPacketChar ^ (beforeDecodeP + pHeader->Code + (i + 1));
		beforeDecodeP = afterDecodeP;

		*pPacketChar = afterparaP ^ (beforeparaP + pHeader->RandKey + (i + 1));
		beforeparaP = afterparaP;

		checksum += *pPacketChar % 256;
	}

	//복호화가 제대로 이루어졌는지 확인
	if (pHeader->CheckSum != checksum)
	{
		printf("[Decode] checksum이 일치하지 않음. 복호화가 제대로 이루어지지 않음.\n");
		return false;
	}
	return true;
}

//작업자 스레드 함수
unsigned int __stdcall CNetServer::WorkerThread(LPVOID arg)
{
	int retval;


	CNetServer* pServer = (CNetServer*)arg;

	//IOCPHandle* iocpHandle = (IOCPHandle*)arg;

	while (1) {
		//비동기 입출력 완료 기다리기
		DWORD cbTransferred;
		SOCKET client_sock;
		SOCKETINFO* ptr;
		OVERLAPPED_CONTEXT* lpOverlapped;
		retval = GetQueuedCompletionStatus(pServer->_hWorkerThreadIOCP, &cbTransferred, (PULONG_PTR)&ptr, (LPOVERLAPPED*)&lpOverlapped, INFINITE);

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
			bool isIOSuccess = WSAGetOverlappedResult(ptr->sock, (LPWSAOVERLAPPED)&lpOverlapped, &lpcbTransfer, false, &temp2);
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

			CRingBuffer* rb = ptr->recvBuf;
			if (rb->MoveRear(cbTransferred) != 0)
			{
				//Recv 버퍼에 있는 내용 읽어서, send링버퍼에 담기
				

				//이제 메시지 길이 단위로 읽어서 컨텐츠 스레드에 넘겨야 한다.
				//그래야 메시지 순서가 보장됨.
				//수신 링버퍼에 있는 데이터 중 헤더 길이만큼 있는 메시지는 모두 읽어서 처리
				//------------------------------------------------------
				// 1. 링 버퍼에서 Dequeue
				// 먼저 메시지 길이만큼 읽을 후, 해당 메시지 길이를 Dequeue
				//------------------------------------------------------
				while (1)
				{
					//메시지 헤더 먼저 읽기
					if (rb->GetUseSize() >= sizeof(MsgHeader))
					{
						MsgHeader header;
						rb->Peek((char*)&header, sizeof(MsgHeader));
						//메시지 페이로드 길이 읽기
						if (rb->GetUseSize() >= sizeof(MsgHeader) + header.Len)
						{
							//디코딩
							if (pServer->Decode(&header, rb->GetFrontBufferPtr()))
							{
								//네트워크 헤더 제거한 나머지 컨텐츠에게 패킷에 담아서 넘겨주기
								CPacket* contentPacket = CPacket::Alloc();
								int ret = contentPacket->PutData(rb->GetFrontBufferPtr() + sizeof(MsgHeader), header.Len);
								if (ret != header.Len)
								{
									printf("[Network] 직렬화 버퍼 삽입 오류: 요청 %d, 실제 %d\n", header.Len, ret);
									__debugbreak();
								}

								rb->MoveFront(sizeof(MsgHeader) + header.Len);
								
								contentPacket->AddRef();
								pServer->OnRecv(ptr->session_id, contentPacket);
								contentPacket->SubRef();
							}
							else
							{
								//디코딩이 실패했다면? 해당 메시지를 그냥 폐기하는게 맞을 것으로 생각함.
								//폐기하고 세션 종료까지 해주는게 맞다고 생각함.
								__debugbreak();
							}
						}
						else
						{
							break;
						}
					}
					else
					{
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
			
			//ptr->GetSessionLock();
			ptr->sendBuf->GetLockBuffer();
			ptr->sendBuf->MoveFront(cbTransferred);
			//패킷 포인터 크기만큼 돌면서 subref를 하면 되지 않을까?
			ptr->sendBuf->UnLockBuffer();
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


bool CNetServer::WsaRecvSession(SOCKADDR_IN& clientaddr, SOCKETINFO*& ptr)
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
		retval = WSARecv(ptr->sock, wsabuf, 2, &recvbytes, &flags, (LPWSAOVERLAPPED)ptr->recvOverlapped, NULL);

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
		retval = WSARecv(ptr->sock, &wsabuf, 1, &recvbytes, &flags, (LPWSAOVERLAPPED)ptr->recvOverlapped, NULL);

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

bool CNetServer::WsaSendSession(SOCKADDR_IN& clientaddr, SOCKETINFO*& ptr)
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

	//세션에 락을 걸기 전에 올바른 송신 진행을 위해 작업한 내용이었는데
	//세션에 락을 걸고 송신을 진행한다는게 보장된다면 없어도 되지 않나? 테스트 필요
	if (InterlockedCompareExchange(&ptr->IsSending, 1, 0) == 0)
	{
		//이미 한발 앞서서 처리된 경우
		//다시 해제 시켜준다.
		//ptr->sendBuf.GetLockBuffer();
		if (ptr->sendBuf->GetUseSize() == 0)
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
		//ptr->sendBuf->UnLockBuffer();

		//printf("[Network] 송신 진행 중 아님, 송신 루트 탐., 포트번호 = %d\n", ntohs(clientaddr.sin_port));
		ptr->sendOverlapped->op = ESend;
		ZeroMemory(ptr->sendOverlapped, sizeof(OVERLAPPED));

		int sendlen = ptr->sendBuf->GetUseSize();
		if (sendlen == 0)
		{
			while (1)
			{
				printf("[Network] 미친 지금 0짜리 보낼뻔\n");
			}
		}
		if (sendlen > ptr->sendBuf->DirectDequeueSize())
		{
			WSABUF wsabuf[2];
			wsabuf[0].buf = ptr->sendBuf->GetFrontBufferPtr();
			wsabuf[0].len = ptr->sendBuf->DirectDequeueSize();
			int frontSize = ptr->sendBuf->GetBufferSize() - ptr->sendBuf->GetFreeSize() - ptr->sendBuf->DirectDequeueSize();
			//wsabuf[1].buf = ptr->sendBuf->GetRearBufferPtr() - frontSize;
			wsabuf[1].buf = ptr->sendBuf->GetBufPtr();
			wsabuf[1].len = frontSize;


			int increase = InterlockedIncrement((long*)&ptr->IOCount);
			if (increase == 1)
			{
				printf("[Network] 누군가 정리 중인 것으로 보임. 송신 진행 불가. 포트번호 = %d\n", ntohs(clientaddr.sin_port));
				//ptr->UnLockSession();
				//누군가가 정리중이라면 send를 진행시키지 않는다.
				return false;
				/*while (1)
				{
					printf("[Network] send완료 시 감소 전이므로 발생해선 안되며, sendPacket에서도 삭제되었음변 아예 송신 못함.\n");
				}*/
			}
			else
			{
				//nterlockedIncrement((long*)&i_send);
			}
			//printf("[Network] 데이터 송신  포트번호 = %d\n", ntohs(clientaddr.sin_port));
			retval = WSASend(ptr->sock, wsabuf, 2, (LPDWORD)&sendlen, 0, (LPWSAOVERLAPPED)ptr->sendOverlapped, NULL);


			if (retval == SOCKET_ERROR)
			{
				if (WSAGetLastError() != WSA_IO_PENDING)
				{
					//printf("[Network] ");
					//err_display("WSASend()");
					/*InterlockedDecrement((long*)&d_send);*/
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
			wsabuf.buf = ptr->sendBuf->GetFrontBufferPtr();
			wsabuf.len = ptr->sendBuf->DirectDequeueSize();

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
			//printf("[Network] 데이터 송신  포트번호 = %d\n", ntohs(clientaddr.sin_port));
			retval = WSASend(ptr->sock, &wsabuf, 1, (LPDWORD)&sendlen, 0, (LPWSAOVERLAPPED)ptr->sendOverlapped, NULL);


			if (retval == SOCKET_ERROR)
			{
				if (WSAGetLastError() != WSA_IO_PENDING)
				{
					//printf("[Network] ");
					//err_display("WSASend()");
					//InterlockedDecrement((long*)&d_send);
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

