#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "ChattingServer.h"
#include "CPacket.h"
#include "CommonProtocol.h"

#define SERVERPORT (6000)
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

ChattingServer::ChattingServer()
{
	//네트워크, 컨텐츠 스레드 입출력 완료 포트 생성
	contentHcp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (contentHcp == NULL)
	{
		printf("[ChattingServer] 컨텐츠 스레드 IOCP 생성 실패");
		return;
	}

	ServerAndHandle* sah = new ServerAndHandle;
	sah->handle = contentHcp;
	sah->thisptr = this;

	unsigned int uiThreadID;
	HANDLE hThread = (HANDLE)_beginthreadex(
		NULL,          
		0,              
		ContentsThread,   
		sah,    // 스레드에게 this포인터와 IOCP핸들 인자로 전달
		0,              
		&uiThreadID    
		);

		if (hThread == NULL) 
		{
			printf("[ChattingServer] 컨텐츠 스레드 생성 실패");
			return;
		}

		CloseHandle(hThread);
}

ChattingServer::~ChattingServer()
{

}

//----------------------
// 특정 대역의 IP를 차단하고 싶을때, 들어온 IP와 일치하면 block
// 특정 대역의 IP만 허용시키고 싶을때, 들어온 IP와 일치하면 true
//----------------------
bool ChattingServer::OnConnectionRequest(std::string IP, int Port)
{
    //if (_serverMode == QA)
    //{

    //}
    //else
    //{

    //}
    return true;
}

void ChattingServer::OnClientJoin(SOCKADDR_IN clientaddr, SessionID s)
{
    //printf("[Echo 서버] 클라이언트 접속 : IP 주소 = %s, 포트번호 = %d\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
}

void ChattingServer::OnClientLeave(SessionID s)
{
    //캐릭터 삭제같은걸 넣으면 될 것 같은데 에코에선 딱히 할게 없는 것으로 보임.
}

//컨텐츠 스레드 하나를 IOCP를 사용해서 깨운 후, 메시지를 전달하면 될 것으로 보임.
//해당 스레드는 깨어나서 메시지 종류에 따라 switch-case로 채팅 로직을 처리하면 될 것.
void ChattingServer::OnRecv(SessionID sessionID, CPacket* pPacket)
{
	//네트워크에서 넘긴 길이만큼 추출
	//Msg recvMsg; 굳이 동적할당 해야할까? 어차피 송신 링버퍼에 복사가 되었다면 문제 없는게 정상임.
	Msg* recvMsg = new Msg;

	//이런식으로 GetData가 아니라 패킷에서 긁어서 넣어줘야 할 것 같은데..
	int len = pPacket->GetDataSize();
	int ret = pPacket->GetData(recvMsg->payload, len);
	if (ret == 0)
	{
		while (1)
		{
			printf("[Contents] 패킷에 남은 메시지 없는데 추출 시도함.\n");
		}
	}
	else if (len != sizeof(recvMsg->payload))
	{
		while (1)
		{
			//악의적인 클라로 간주하고 끊는게 맞음. Disconnect하면 될 듯
			printf("[Contents] 메시지와 패킷의 양식이 다름, 메시지  : %d , 패킷 : %d \n", sizeof(recvMsg->payload), len);
		}
	}
	else if(len != ret)
	{
		while (1)
		{
			//악의적인 클라로 간주하고 끊는게 맞음. Disconnect하면 될 듯
			printf("[Contents] 패킷에서 추출한 크기가 예상과 다름. 요청  : %d , 실제 : %d \n", len, ret);
		}
	}

	//printf("[Contents] : 수신 메시지 내용 %lld\n", (long long)recvMsg->payload);

	//추출한 메시지에 헤더를 붙여서 SendPakcet
	recvMsg->header = sizeof(recvMsg->payload);

	CPacket* pSendPacket = new CPacket(sizeof(Msg));
	pSendPacket->operator<<(recvMsg->header);
	pSendPacket->PutData(recvMsg->payload, recvMsg->header);

	int sendret = SendPacket(sessionID, pSendPacket);
	delete pSendPacket;

	//세션이 이제 여기서 삭제되는 경우는 없음.
	if (sendret == 0)
	{
		while (1)
		{
			printf("[Contents] 네트워크 송신 버퍼가 꽉 참\n");
		}
	}

	delete recvMsg;
}

void ChattingServer::OnError(int errorcode, char*)
{

}

//작업자 스레드 함수
unsigned int __stdcall ChattingServer::ContentsThread(LPVOID arg)
{
	int retval;

	ServerAndHandle* sah = (ServerAndHandle*)arg;

	CNetServer* pServer = sah->thisptr;
	HANDLE hcp = sah->handle;

	//IOCPHandle* iocpHandle = (IOCPHandle*)arg;

	while (1) {
		//비동기 입출력 완료 기다리기
		DWORD cbTransferred;
		SOCKET client_sock;
		long long session_id;
		CPacket* pPacket;
		retval = GetQueuedCompletionStatus(hcp, &cbTransferred, (PULONG_PTR)&session_id, (LPOVERLAPPED*)&pPacket, INFINITE);

		//비동기 입출력 결과 확인
		if (cbTransferred == 0)
		{
			//서버가 0짜리 던져줌.
			//나중에 종료 코드로 사용할 수 있음.
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

		if (pPacket->GetDataSize() <= 0)
		{
			while (1)
			{
				printf("[Contents] 아무것도 없는 패킷이 넘어옴\n");
			}
		}

		WORD type;
		*pPacket >> type;

		CPacket packetToSend;

		switch (type)
		{
		//------------------------------------------------------------
		// 채팅서버 로그인 요청
		// 1. 중복 로그인 확인
		// 2. 플레이어 맵에 추가
		// 3. 로그인 확인 메시지 전송
		//------------------------------------------------------------
		case en_PACKET_CS_CHAT_REQ_LOGIN:
		{
			Character* pcharacter = new Character;

			*pPacket >> pcharacter->_AccountNo;
			pPacket->GetData((char*)pcharacter->_ID, sizeof(pcharacter->_ID));// null 포함
			pPacket->GetData((char*)pcharacter->_Nickname, sizeof(pcharacter->_Nickname));// null 포함
			pPacket->GetData(pcharacter->_SessionKey, sizeof(pcharacter->_SessionKey));// 인증토큰


			BYTE	Status;			// 채팅서버 로그인 응답 0:실패	1:성공
			//1.
			if (mapCharacter[pcharacter->_AccountNo] != nullptr)
			{
				printf("[en_PACKET_CS_CHAT_REQ_LOGIN] 중복 로그인 감지\n");
				Status = 0;
			}
			else
			{
				// 2.
				mapCharacter[pcharacter->_AccountNo] = pcharacter;
				Status = 1;
			}

			INT64	AccountNo = pcharacter->_AccountNo;
			// 3. 
			packetToSend.PutData((char*)en_PACKET_SC_CHAT_RES_LOGIN, sizeof(WORD));
			packetToSend.PutData((char*)Status, sizeof(BYTE));
			packetToSend.PutData((char*)AccountNo, sizeof(INT64));

			break;
		}


		//------------------------------------------------------------
		// 채팅서버 섹터 이동 요청
		// 1. 플레이어가 맵에 있는지 확인
		// 2. 섹터 결과 대입
		//   a. 원래 있던 섹터에서 플레이어 삭제
		//   b. 새로운 섹터에 플레이어 대입
		// 3. 결과 반환
		//------------------------------------------------------------
		case en_PACKET_CS_CHAT_REQ_SECTOR_MOVE:
		{
			INT64	AccountNo;
			WORD	SectorX;
			WORD	SectorY;

			//1.
			if (mapCharacter[AccountNo] != nullptr)
			{
				// 2. 
				Character* pcharacter = mapCharacter[AccountNo];
				//a.
				auto a = umapCharcterSector[pcharacter->_SectorY][pcharacter->_SectorX].find(pcharacter->_AccountNo);
				//------------------------하는 일------------------------
				// 제일 처음 생성된 플레이어는 섹터 좌표를 받고있지 않은데 어떻게 삭제하지?
				// 
				//b.
				pPacket->GetData((char*)pcharacter->_SectorX, sizeof(SectorX));
				pPacket->GetData((char*)pcharacter->_SectorY, sizeof(SectorY));

				// 3.
				packetToSend.PutData((char*)en_PACKET_SC_CHAT_RES_SECTOR_MOVE, sizeof(WORD));
				packetToSend.PutData((char*)AccountNo, sizeof(AccountNo));
				packetToSend.PutData((char*)pcharacter->_SectorX, sizeof(SectorX));
				packetToSend.PutData((char*)pcharacter->_SectorY, sizeof(SectorY));
		   }
		   else
		   {
				printf("[en_PACKET_CS_CHAT_REQ_SECTOR_MOVE] 맵에 없는 플레이어에 대해 섹터 이동 요청\n");
		   }

			break;
		}
		//------------------------------------------------------------
		// 채팅서버 채팅보내기 요청
		// 1. 계정을 플레이어 맵에서 확인
		// 2. 메시지를 주위 섹터 플레이어에게 보내기
		//------------------------------------------------------------
		case en_PACKET_CS_CHAT_REQ_MESSAGE:
		{
			INT64	AccountNo;
			WORD	MessageLen;
			WCHAR	Message[MessageLen / 2];		// null 미포함
			break;
		}


		//------------------------------------------------------------
		// 채팅서버 채팅보내기 응답  (다른 클라가 보낸 채팅도 이걸로 받음)
		//------------------------------------------------------------
		case en_PACKET_SC_CHAT_RES_MESSAGE:
		{
			INT64	AccountNo;
			WCHAR	ID[20];					// null 포함
			WCHAR	Nickname[20];				// null 포함
		
			WORD	MessageLen;
			WCHAR	Message[MessageLen / 2];		// null 미포함
			break;
		}
			

		//------------------------------------------------------------
		// 하트비트
		// 클라이언트는 이를 30초마다 보내줌.
		// 서버는 40초 이상동안 메시지 수신이 없는 클라이언트를 강제로 끊어줘야 함.
		//------------------------------------------------------------	
		case en_PACKET_SC_CHAT_REQ_HEARTBEAT:
		{
			WORD	Type;
			break;
		}

		default:
			break;
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
		else if (sendret != sizeof(Msg))
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
