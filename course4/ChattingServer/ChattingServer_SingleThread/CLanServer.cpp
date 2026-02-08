#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma comment(lib, "ws2_32")

#include "CLanServer.h"
#include "CommonProtocol.h"
#include "cSessionMap.h"
#include "Session.h"
#include "CRingBuffer.h"
#include  "CPacketRingBuffer.h"
#include "OVERLAPPED_CONTEXT.h"
#include "MessageQueue.h"
#include "errlog.h"
#include "CPacketForMultiThread.h"
#include <ws2tcpip.h>


#define SERVERPORT (6000)
#define BUFSIZE (1024 * 16)
#define MSG_SIZE (8)
#define WSASEND_MAX_BUFFER_COUNT (128)

#define RELEASE_FLAGBIT 31
#define RELEASE_FLAG      (1u << RELEASE_FLAGBIT)  
#define RELEASE_FLAG_MASK (RELEASE_FLAG - 1)       

//------------------------------------
//메시지 프로토콜
// 헤더 2Byte (길이)
// 데이터 8Byte(에코)
//------------------------------------
//struct Msg {
//	short header = 0;
//	char payload[MSG_SIZE] = {};
//};



bool CLanServer::Decode(PacketHeader* pHeader, char* pc)
{
	unsigned int checksum = 0;
	unsigned char beforeparaP = 0;
	unsigned char afterparaP = 0;
	unsigned char encodeP = 0;


	int payLoadSize = pHeader->Len;

	int checkSumSize = sizeof(PacketHeader::CheckSum);
	char* pPacketChar = &pc[sizeof(PacketHeader) - checkSumSize];

	afterparaP = *pPacketChar ^ (encodeP + dfPACKET_KEY + 1);
	encodeP = *pPacketChar;

	*pPacketChar = afterparaP ^ (beforeparaP + pHeader->RandKey + 1);
	beforeparaP = afterparaP;

	for (int i = 0; i < payLoadSize; i++)
	{
		char* pPacketChar = &pc[sizeof(PacketHeader) + i];

		afterparaP = *pPacketChar ^ (encodeP + dfPACKET_KEY + (i + 2));
		encodeP = *pPacketChar;

		*pPacketChar = afterparaP ^ (beforeparaP + pHeader->RandKey + (i + 2));
		beforeparaP = afterparaP;

		checksum += *pPacketChar % 256;
		checksum %= 256;
	}



	//복호화가 제대로 이루어졌는지 확인
	if (pHeader->CheckSum != checksum)
	{
		printf("[Decode] checksum이 일치하지 않음. 복호화가 제대로 이루어지지 않음.\n");
		return false;
	}
	return true;
}

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

		_hWorkerThreadIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
		if (_hWorkerThreadIOCP == NULL) return false;

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
				this,    // Argument list to be passed to thread function
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
		printf("서버 시작 포트 : %d\n", SERVERPORT);

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

			
			//여기서 세션의 네트워크 정보를 넘겨서 세팅하게 만든다.
			SOCKETINFO* ptr = sessionMap->AllocSessionptr(client_sock);

			char ipStr[INET_ADDRSTRLEN];
			inet_ntop(AF_INET, &clientaddr.sin_addr, ipStr, sizeof(ipStr));
			ptr->_IP = ipStr;
			ptr->_PORT = ntohs(clientaddr.sin_port);


			//세션의 세션 ID를 찾는 함수를 세션에 만들어야 겠다.
			uint64_t id_bit = ptr->_sessionKey.GetSessionId();
			WSABUF wsabuf;
			wsabuf.buf = ptr->_recvBuf->GetFrontBufferPtr();
			wsabuf.len = ptr->_recvBuf->GetFreeSize();

			//printf("[TCP 서버] 클라이언트 접속 : IP 주소 = %s, 포트번호 = %d\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
			printf("[TCP 서버] 클라이언트 접속 : ID = %d\n", id_bit);
			OnClientJoin(clientaddr, ptr->_sessionKey);

			//소켓과 입출력 완료 포트 연결
			CreateIoCompletionPort((HANDLE)client_sock, _hWorkerThreadIOCP, (ULONG_PTR)ptr, 0);

			//비동기 입출력 시작
			flags = 0;
			if (!sessionMap->IncreaseSessionIO(ptr))
			{
				__debugbreak();
			}
			retval = WSARecv(client_sock, &wsabuf, 1, &recvbytes, &flags, (LPWSAOVERLAPPED)ptr->_recvOverlapped, NULL);
			if (retval == SOCKET_ERROR)
			{
				if (WSAGetLastError() != ERROR_IO_PENDING) {
					err_display("WSARECV()");

					//여기서 IOCount의 최상위 비트를 SessionReleaseFlag로 사용한다면??
					//IOCount가 0이면 비트에 1넣기
					if (!sessionMap->DecreaseSessionIO(ptr))
					{
						//세션이 삭제된 경우
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

bool CLanServer::Disconnect(SessionKey sessionId)
{
	SOCKETINFO* ptr;
	cSessionMap* pSessionMap = cSessionMap::GetSessionMap();

	ptr = pSessionMap->GetSessionptr(sessionId);
	if (ptr == nullptr)
	{
		pSessionMap->DecreaseSessionIO(ptr);
		return false;
	}
	if (shutdown(ptr->_sock, SD_RECEIVE) != 0)
	{
		pSessionMap->DecreaseSessionIO(ptr);
		return false;
	}

	pSessionMap->DecreaseSessionIO(ptr);
    return true;
}

bool CLanServer::SendPacket(SessionKey sessionId, CPacket* cp)
{
	//1. 세션 검색
	SOCKETINFO* ptr;
	cSessionMap* pSessionMap = cSessionMap::GetSessionMap();

	ptr = pSessionMap->GetSessionptr(sessionId);
	if (ptr == nullptr)
	{
		printf("[Network] 삭제된 세션입니다~ 접근 불가능~\n");
		pSessionMap->DecreaseSessionIO(ptr);
		return false;
	}

	//2. 인코딩
	//함수 안에서 네트워크 헤더까지 세팅
	cp->Encode();

	//3. 링버퍼 삽입
	cp->AddRef();
	int ret = ptr->_sendBuf->Enqueue(cp);
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
	
	if (!CanSend(ptr))
	{
		//printf("[Network] 송신 중입니다~\n");
		pSessionMap->DecreaseSessionIO(ptr);
		return true;
	}

	if (!SendPost(clientaddr, ptr))
	{
		//안에서 세션 삭제가 일어난 경우 바로 GQCS 대기 루틴
		printf("[Network] WsaSendSession 실패했어요~\n");
		__debugbreak();
		pSessionMap->DecreaseSessionIO(ptr);
		return false;
	}

	//PostQueuedCompletionStatus(pIOCPHandle->netHcp, len, (ULONG_PTR)ptr, (LPWSAOVERLAPPED)&ptr->contentsOverlapped);
	//ptr->UnLockSession();
	pSessionMap->DecreaseSessionIO(ptr);
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


//void CLanServer::ReleaseSession(SOCKADDR_IN& clientaddr, SOCKETINFO* ptr)
//{
//	cSessionMap* psm = cSessionMap::GetSessionMap();
//	psm->FreeSession(ptr, inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
//}
//
//bool CLanServer::DecreaseSessionIO(SOCKADDR_IN& clientaddr, SOCKETINFO* ptr)
//{
//	unsigned long oldVal;
//	unsigned long newVal;
//
//	while (true)
//	{
//		oldVal = ptr->IOCount;
//
//		// 이미 Release 상태면 아무것도 하지 않음
//		if (oldVal & RELEASE_FLAG)
//			return false;
//
//		unsigned long io = oldVal & RELEASE_FLAG_MASK;
//		if (io == 0)
//		{
//			__debugbreak(); // underflow
//			return false;
//		}
//
//		newVal = oldVal - 1;
//
//		// IOCount 감소 성공?
//		if (InterlockedCompareExchange(
//			(unsigned long*)&ptr->IOCount,
//			newVal,
//			oldVal) == oldVal)
//		{
//			break;
//		}
//	}
//
//	// 감소 후 IOCount == 0 이고 ReleaseFlag == 0 이면
//	if ((newVal & RELEASE_FLAG_MASK) == 0)
//	{
//		// ReleaseFlag 세팅 시도
//		if (InterlockedCompareExchange(
//			(unsigned long*)&ptr->IOCount,
//			newVal | RELEASE_FLAG,
//			newVal) == newVal)
//		{
//			ReleaseSession(clientaddr, ptr);
//			return false;
//		}
//	}
//
//	return true;
//}
//
//bool CLanServer::IncreaseSessionIO(SOCKETINFO* ptr)
//{
//	unsigned long oldVal;
//	unsigned long newVal;
//
//	while (true)
//	{
//		oldVal = ptr->IOCount;
//
//		// 이미 Release 상태면 IO 추가 불가
//		if (oldVal & RELEASE_FLAG)
//			return false;
//
//		unsigned long io = oldVal & RELEASE_FLAG_MASK;
//		if (io == RELEASE_FLAG_MASK)
//		{
//			__debugbreak(); // overflow
//			return false;
//		}
//
//		newVal = oldVal + 1;
//
//		if (InterlockedCompareExchange(
//			(unsigned long*)&ptr->IOCount,
//			newVal,
//			oldVal) == oldVal)
//		{
//			return true;
//		}
//	}
//}

bool CLanServer::CanSend(SOCKETINFO* ptr)
{
	// 이미 누군가 Send 중이면 절대 허용 X
	if (InterlockedCompareExchange(&ptr->_IsSending, 1, 0) != 0)
		return false;

	// 내가 Send 담당자가 됐는데 보낼 게 없다?
	if (ptr->_sendBuf->GetUseSize() == 0)
	{
		InterlockedExchange(&ptr->_IsSending, 0);
		return false;
	}

	return true;
}

//작업자 스레드 함수
unsigned int __stdcall CLanServer::WorkerThread(LPVOID arg)
{
	int retval;

	cSessionMap* sessionMap = cSessionMap::GetSessionMap();

	CLanServer* pServer = (CLanServer*)arg;


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
		getpeername(ptr->_sock, (SOCKADDR*)&clientaddr, &addrlen);

		//비동기 입출력 결과 확인
		if (cbTransferred == 0)
		{
			//클라가 종료신호 FIN보냄.
			//printf("[Network] 클라이언트 종료 신호 수신: IP 주소 = %s, 포트번호 = %d\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
			if (!sessionMap->DecreaseSessionIO(ptr))
			{
				//세션이 삭제된 경우
				continue;
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
				if (!sessionMap->DecreaseSessionIO(ptr))
				{
					//세션이 삭제된 경우
					continue;
				}
			}
			else
			{
				//IO 실패
				printf("[Network] IO 실패\n");
				if (!sessionMap->DecreaseSessionIO(ptr))
				{
					//세션이 삭제된 경우
					continue;
				}

			}

			continue;
		}

		if (lpOverlapped->op == ERecv)
		{
			//printf("[Network] 클라이언트 수신, 포트번호 = %d\n", ntohs(clientaddr.sin_port));

			CRingBuffer* rb = ptr->_recvBuf;
			if (rb->MoveRear(cbTransferred) == 0)
			{
				//수신 링버퍼가 가득차서 더이상 데이터를 받을 수 없는 상황
				//cpu 100%인지 확인하기 -> 아니라면 로직 오류, 맞을 경우 AcceptThread의 부하 줄일 수 있는 방법 고려해야 함.
				printf("[Network] 수신 링버퍼 꽉 찼음.\n");
				__debugbreak();
			}

			//Recv 버퍼에 있는 내용 읽어서, send링버퍼에 담기

			//이제 메시지 길이 단위로 읽어서 컨텐츠 스레드에 넘겨야 한다.
			//그래야 메시지 순서가 보장됨.
			//수신 링버퍼에 있는 데이터 중 헤더 길이만큼 있는 메시지는 모두 읽어서 처리
			//------------------------------------------------------
			// 1. 링 버퍼에서 Dequeue
			// 먼저 메시지 길이만큼 읽을 후, 해당 메시지 길이를 Dequeue
			//------------------------------------------------------

			//메시지 헤더 먼저 읽기
			while (rb->GetUseSize() >= sizeof(PacketHeader))
			{
				PacketHeader* header = (PacketHeader*)rb->GetFrontBufferPtr();

				if (header->Code != dfPACKET_CODE)
				{
					//잘못된 패킷이니까 세션 죽이자.
					__debugbreak();
				}

				//메시지 페이로드 길이 읽기
				if (rb->GetUseSize() < sizeof(PacketHeader) + header->Len)
				{
					break;
				}

				//디코딩
				if (!pServer->Decode(header, rb->GetFrontBufferPtr()))
				{
					//디코딩 실패
					__debugbreak();
				}

				//네트워크 헤더 제거한 나머지 컨텐츠에게 패킷에 담아서 컨텐츠에 넘겨주기
				CPacket* contentPacket = CPacket::Alloc();
				int ret = contentPacket->PutData(rb->GetFrontBufferPtr() + sizeof(PacketHeader), header->Len);
				if (ret != header->Len)
				{
					printf("[Network] 직렬화 버퍼 삽입 오류: 요청 %d, 실제 %d\n", header->Len, ret);
					__debugbreak();
				}

				rb->MoveFront(sizeof(PacketHeader) + header->Len);

				contentPacket->AddRef();
				pServer->OnRecv(ptr->_sessionKey, contentPacket);
				contentPacket->SubRef();
			}

			//현재 스레드를 다시 Recv 등록하기
			//printf("[Network] 다시 recv 대기하기, 포트번호 = %d\n", ntohs(clientaddr.sin_port));
			if (!pServer->WsaRecvSession(clientaddr, ptr))
			{
				//안에서 세션 삭제가 일어난 경우 바로 GQCS 대기 루틴
				continue;
			}

			//GQCS Recv 완료통지에 대한 IO 감소
			if (!sessionMap->DecreaseSessionIO(ptr))
			{
				//세션이 삭제된 경우
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
			int sendPacketNum = ptr->_sendPacketNum;
			int srfront = ptr->_sendBuf->GetFront();
			int cpysrfront = srfront;
			int srCapacity = ptr->_sendBuf->GetBufferSize();
			CPacket** ppacket = ptr->_sendBuf->GetBufPtr();

			//패킷 해제
			for (int i = 0; i < sendPacketNum; i++)
			{
				ppacket[cpysrfront]->SubRef();
				cpysrfront = (cpysrfront + 1) % srCapacity;
			}

			//ptr->GetSessionLock();
			ptr->_sendBuf->MoveFront(sendPacketNum);
			//ptr->UnLockSession();
			
			//송신 완료, 송신 플래그 해제
			if (InterlockedCompareExchange(&ptr->_IsSending, 0, 1) != 1)
			{
				printf("Send 중첩 발생, 세션 ID = %llu\n", ptr->_sessionKey.GetSessionId());
				__debugbreak();
			}

			//ptr->GetSessionLock();
			//송신 링버퍼에 남은 데이터를 Send
			//ptr->sendBuf->GetLockBuffer();
			if (pServer->CanSend(ptr))
			{
				if (!pServer->SendPost(clientaddr, ptr))
				{
					//안에서 세션 삭제가 일어난 경우 바로 GQCS 대기 루틴
					//ptr->sendBuf->UnLockBuffer();
					continue;
				}
			}

			//ptr->sendBuf->UnLockBuffer();
			//ptr->UnLockSession();

			//GQCS Send 완료통지에 대한 IO 감소
			if (!sessionMap->DecreaseSessionIO(ptr))
			{
				//세션이 삭제된 경우
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


bool CLanServer::WsaRecvSession(SOCKADDR_IN& clientaddr, SOCKETINFO* ptr)
{
	int retval;

	ptr->_recvOverlapped->op = ERecv;
	ZeroMemory(ptr->_recvOverlapped, sizeof(OVERLAPPED));
	

	cSessionMap* sessionMap = cSessionMap::GetSessionMap();

	//Direct로 넣을 수 있냐 없냐에 따라 여러 버퍼로 나눠서 받아야 함.
	int recvlen = ptr->_recvBuf->GetFreeSize();
	if (recvlen > ptr->_recvBuf->DirectEnqueueSize())
	{
		WSABUF wsabuf[2];
		wsabuf[0].buf = ptr->_recvBuf->GetRearBufferPtr();
		wsabuf[0].len = ptr->_recvBuf->DirectEnqueueSize();
		int frontSize = ptr->_recvBuf->GetFreeSize() - ptr->_recvBuf->DirectEnqueueSize();
		wsabuf[1].buf = ptr->_recvBuf->GetFrontBufferPtr() - frontSize;
		wsabuf[1].len = frontSize;
		DWORD recvbytes;
		DWORD flags = 0;
		if (!sessionMap->IncreaseSessionIO(ptr))
		{
			__debugbreak();
		}
		retval = WSARecv(ptr->_sock, wsabuf, 2, &recvbytes, &flags, (LPWSAOVERLAPPED)ptr->_recvOverlapped, NULL);

		if (retval == SOCKET_ERROR)
		{
			if (WSAGetLastError() != WSA_IO_PENDING)
			{
				//printf("[Network] ");
				//err_display("WSARecv()");
				if (!sessionMap->DecreaseSessionIO(ptr))
				{
					//세션이 삭제된 경우
					return false;
				}
			}
		}

	}
	else
	{
		WSABUF wsabuf;
		wsabuf.buf = ptr->_recvBuf->GetRearBufferPtr();
		wsabuf.len = ptr->_recvBuf->DirectEnqueueSize();
		DWORD recvbytes;
		DWORD flags = 0;
		if (!sessionMap->IncreaseSessionIO(ptr))
		{
			__debugbreak();
		}
		retval = WSARecv(ptr->_sock, &wsabuf, 1, &recvbytes, &flags, (LPWSAOVERLAPPED)ptr->_recvOverlapped, NULL);

		if (retval == SOCKET_ERROR)
		{
			if (WSAGetLastError() != WSA_IO_PENDING)
			{
				//printf("[Network] ");
				//err_display("WSARecv()");
				if (!sessionMap->DecreaseSessionIO(ptr))
				{
					//세션이 삭제된 경우
					return false;
				}
			}
		}
	}

	return true;
}

bool CLanServer::SendPost(SOCKADDR_IN& clientaddr, SOCKETINFO* ptr)
{
	if (ptr == nullptr)
	{
		printf("[WsaSendSession] 송신 시도중인 세션이 이미 삭제된 세션\n");
		__debugbreak();
	}

	CPacketRingBuffer* prb = ptr->_sendBuf;
	int retval;
	//Send 중이 아니라면
	//Send 링버퍼에 있는 있는 내용 전부 Send

	ptr->_sendOverlapped->op = ESend;
	ZeroMemory(ptr->_sendOverlapped, sizeof(OVERLAPPED));

	int remain = prb->GetUseSize();

	if (remain == 0)
	{
		printf("[WsaSendSession] 미친 지금 0짜리 보낼뻔\n");
		__debugbreak();
	}

	WSABUF wsabuf[WSASEND_MAX_BUFFER_COUNT] = { 0, };

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

	cSessionMap* sessionMap = cSessionMap::GetSessionMap();
	if (!sessionMap->IncreaseSessionIO(ptr))
	{
		printf("[Network] 누군가 정리 중인 것으로 보임. 송신 진행 불가. 포트번호 = %d\n", ntohs(clientaddr.sin_port));
		__debugbreak();
		return false;
	}

	if (wsabuf[0].len == 0)
	{
		printf("[WsaSendSession] wsabuf 에 아무 값도 안들어갔음.\n");
		__debugbreak();
	}

	DWORD sendBytes = 0;
	//printf("[Network] 데이터 송신  포트번호 = %d\n", ntohs(clientaddr.sin_port));

	ptr->_sendPacketNum = remain;
	retval = WSASend(ptr->_sock, wsabuf, remain, (LPDWORD)&sendBytes, 0, (LPWSAOVERLAPPED)ptr->_sendOverlapped, NULL);

	if (retval == SOCKET_ERROR)
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			if (!sessionMap->DecreaseSessionIO(ptr))
			{
				//세션이 삭제된 경우
				return false;
			}
		}
	}


	return true;
}

