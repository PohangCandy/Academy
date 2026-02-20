#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "ChattingServer.h"
#include "CPacketForMultiThread.h"
#include "CommonProtocol.h"
#include "MemoryPoolForLockFree.h"

#define SERVERPORT (6000)
#define BUFSIZE (1024 * 1024)
#define MSG_SIZE (8)

//채팅 서버에 로그인한 캐릭터를 저장해둔 맵
std::unordered_map<uint64_t, Character*> umapCharacter;

//캐릭터 리스트를 담아둔 섹터 맵
std::unordered_map<uint64_t, Character*> umapCharcterSector[50][50];

myMemorypool::CMemoryPool<Character> characterpool(10000,true);

ChattingServer::ChattingServer()
{
	//네트워크, 컨텐츠 스레드 입출력 완료 포트 생성
	hContentCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (hContentCompletionPort == NULL)
	{
		printf("[ChattingServer] 컨텐츠 스레드 IOCP 생성 실패");
		__debugbreak();
		return;
	}

	//ServerAndHandle* sah = new ServerAndHandle;
	//sah->handle = hContentCompletionPort;
	//sah->thisptr = this;

	//컨텐츠 스레드 생성
	unsigned int uiThreadID;
	hContentThread = (HANDLE)_beginthreadex(
		NULL,          
		0,              
		ContentsThread,   
		this,    // 스레드에게 this포인터와 IOCP핸들 인자로 전달
		0,              
		&uiThreadID    
		);

		if (hContentThread == NULL)
		{
			printf("[ChattingServer] 컨텐츠 스레드 생성 실패");
			__debugbreak();
			return;
		}

		//타이머 스레드 생성
		hTimerThread = (HANDLE)_beginthreadex(
			NULL,
			0,
			TimerThread,
			this,    // 스레드에게 this포인터와 컨텐츠 스레드의 IOCP핸들 인자로 전달
			0,
			&uiThreadID
		);

		if (hTimerThread == NULL)
		{
			printf("[ChattingServer] 타이머 스레드 생성 실패");
			__debugbreak();
			return;
		}

		_bIsTimerThreadAlive = true;
}

ChattingServer::~ChattingServer()
{
	//컨텐츠 스레드 종료 유도
	PostQueuedCompletionStatus(
		hContentCompletionPort,
		0,
		0,
		nullptr
	);

	//컨텐츠 스레드 종료 대기
	WaitForSingleObject(hContentThread, INFINITE);
	CloseHandle(hContentThread);

	//컨텐츠 IOCP 핸들 반납
	CloseHandle(hContentCompletionPort);

	//타이머 스레드 종료 유도
	_bIsTimerThreadAlive = false;

	//타이머 스레드 종료 대기
	WaitForSingleObject(hTimerThread, INFINITE);
	CloseHandle(hTimerThread);
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

void ChattingServer::OnClientJoin(SOCKADDR_IN clientaddr,SessionKey sessionkey)
{
	CPacket* pPacket = CPacket::Alloc();
	pPacket->AddRef();
	*pPacket << en_PACKET_SS_Create_Character;
	if (!PostQueuedCompletionStatus(hContentCompletionPort, pPacket->GetDataSize(), (ULONG_PTR)sessionkey.GetSessionKey(), (LPWSAOVERLAPPED)pPacket))
	{
		printf("[OnClientJoin] 컨텐츠 IOCP에 PQCS실패!\n");
		__debugbreak();
	}
}

void ChattingServer::OnClientLeave(SessionKey sessionkey)
{
    //캐릭터 삭제
	//큐에 캐릭터 삭제 메시지를 넣어 컨텐츠 스레드가 해당 캐릭터를 삭제하도록 만든다.
	//미리 삭제해버리면 컨텐츠 스레드 큐에 남아있는 메시지로 인해 삭제된 캐릭터에 접근하게 되버릴 수 있다.
	//-> 에코는 따로 스레드가 없어서 문제가 없었네..
	CPacket* pPacket = CPacket::Alloc();
	pPacket->AddRef();
	*pPacket << en_PACKET_SS_Session_Release;
	//printf("[OnClientLeave] ID = %lld\n", sessionkey.GetSessionId());
	if (!PostQueuedCompletionStatus(hContentCompletionPort, pPacket->GetDataSize(), (ULONG_PTR)sessionkey.GetSessionKey(), (LPWSAOVERLAPPED)pPacket))
	{
		printf("[OnClientLeave] 컨텐츠 IOCP에 PQCS실패!\n");
		__debugbreak();
	}
}

//컨텐츠 스레드 깨우기
void ChattingServer::OnRecv(SessionKey sessionkey, CPacket* pPacket)
{
	pPacket->AddRef();

	if (!PostQueuedCompletionStatus(hContentCompletionPort, pPacket->GetDataSize(), (ULONG_PTR)sessionkey.GetSessionKey(), (LPWSAOVERLAPPED)pPacket))
	{
		printf("[OnRecv] 컨텐츠 IOCP에 PQCS실패!\n");
		__debugbreak();
	}
}

void ChattingServer::OnError(int errorcode, char*)
{

}

Character* ChattingServer::FindCharacter(SessionKey sessionkey)
{
	auto a = umapCharacter.find(sessionkey.GetSessionId());
	if (a == umapCharacter.end())
	{
		//로그인 요청했는데 캐릭터가 없는 경우? 있을 수 없음.
		__debugbreak();
		return nullptr;
	}

	return a->second;
}

bool ChattingServer::DeleteCharacter(SessionKey sessionkey)
{
	Character* pcharacter = FindCharacter(sessionkey);
	if (pcharacter == nullptr)
		return false;

	// 1. 섹터 맵에서 제거
	if (pcharacter->_SectorY != 0xffff)
	{
		auto& sectorMap = umapCharcterSector[pcharacter->_SectorY][pcharacter->_SectorX];
		sectorMap.erase(pcharacter->_sessionkey.GetSessionId());
	}

	// 2. 전체 캐릭터 맵에서 제거
	umapCharacter.erase(pcharacter->_sessionkey.GetSessionId());

	// 3. 메모리 풀로 반환
	characterpool.Free(pcharacter);
}

bool ChattingServer::CreateCharacter(SessionKey sessionkey)
{
	//이거 네트워크 스레드가 돌려도 되나? 경합 조심해야겠는데
	//컨텐츠가 만들게 해줘야겠는데?
	// 1. 캐릭터 생성
	Character* pcharacter = characterpool.Alloc();
	pcharacter->OnReuse();
	pcharacter->_sessionkey = sessionkey;

	umapCharacter.emplace(sessionkey.GetSessionId(), pcharacter);
	return true;
}

//컨텐츠 스레드 함수
unsigned int __stdcall ChattingServer::ContentsThread(LPVOID arg)
{
	int retval;

	ChattingServer* pServer = (ChattingServer*)arg;

	//bool bremoveDieCharacter = false;

	PacketHeader header;

	while (1) {
		//비동기 입출력 완료 기다리기
		DWORD cbTransferred;
		SOCKET client_sock;
		SessionKey sessionkey;

		CPacket* pPacket = nullptr;
		retval = GetQueuedCompletionStatus(pServer->hContentCompletionPort, &cbTransferred, (PULONG_PTR)&sessionkey, (LPOVERLAPPED*)&pPacket, INFINITE);


		//컨텐츠 스레드 종료
		if (cbTransferred == 0 && (&sessionkey) == nullptr && pPacket == nullptr)
		{
			//모든 섹터 정리
			for (int i = 0; i < 50; i++)
			{
				for (int j = 0; j < 50; j++)
				{
					umapCharcterSector[i][j].clear();
				}
			}

			//모든 캐릭터 정리
			for (auto it = umapCharacter.begin(); it != umapCharacter.end(); )
			{
				Character* pcharacter = it->second;
				characterpool.Free(pcharacter);
				++it;
			}
			//모든 맵 정리
			umapCharacter.clear();


			break;
		}
		else if (retval == 0)
		{
			while (1)
			{
				printf("[Contents] 비동기 함수를 호출하지 않고는 나올 수 없는 경우\n");
			}
		}

		//if (pPacket->GetDataSize() <= 0)
		//{
		//	while (1)
		//	{
		//		printf("[Contents] 아무것도 없는 패킷이 넘어옴\n");
		//	}
		//}

		WORD type;
		*pPacket >> type;

		

		switch (type)
		{
		//------------------------------------------------------------
		// 채팅서버 로그인 요청
		// 1. 캐릭터 검색
		// 2. 토큰 확인(아마 추후)
		// 3. 로그인 확인 메시지 전송
		//------------------------------------------------------------
		case en_PACKET_CS_CHAT_REQ_LOGIN:
		{
			//1. 캐릭터 검색
			Character* pcharacter = pServer->FindCharacter(sessionkey);
			if (pcharacter == nullptr)
			{
				__debugbreak();
			}

			*pPacket >> pcharacter->_AccountNo; 
			pPacket->GetData((char*)pcharacter->_ID, sizeof(pcharacter->_ID));
			pPacket->GetData((char*)pcharacter->_Nickname, sizeof(pcharacter->_Nickname));
			pPacket->GetData(pcharacter->_Token, sizeof(pcharacter->_Token));
			pPacket->SubRef();

			// 2. 토큰 확인(아마 추후)
			BYTE	Status = 1;			// 채팅서버 로그인 응답 0:실패	1:성공
			//여기에 발급된 토큰을 비교하는 작업이 들어가고 실패하면 0인듯? 일단 지금은 무조건 1

			// 3. 로그인 확인 메시지 전송
			CPacket* packetToSend = CPacket::Alloc();
			packetToSend->_MsgheaderSize = sizeof(PacketHeader);
			packetToSend->PutData((char*)&header, sizeof(PacketHeader));
			*packetToSend << (short)en_PACKET_SC_CHAT_RES_LOGIN;
			*packetToSend << (BYTE)Status;
			*packetToSend << (INT64)pcharacter->_AccountNo;

			packetToSend->AddRef();
			bool ret = pServer->SendPacket(pcharacter->_sessionkey, packetToSend);
			packetToSend->SubRef();

			if (!ret)
			{
				printf("[Contents] SendPacket 실패, 세션 ID : %lld\n", pcharacter->_sessionkey.GetSessionId());
				//__debugbreak();
			}
			break;
		}


		//------------------------------------------------------------
		// 채팅서버 섹터 이동 요청
		// 1. 플레이어가 맵에 있는지 확인
		// 2. 섹터 결과 대입
		//   a. 원래 있던 섹터에서 플레이어 삭제
		//   b. 새로운 섹터에 플레이어 대입
		// 3. 섹터 이동 결과 송신 패킷에 삽입
		//------------------------------------------------------------
		case en_PACKET_CS_CHAT_REQ_SECTOR_MOVE:
		{


			//1. 플레이어가 맵에 있는지 확인
			Character* pcharacter = pServer->FindCharacter(sessionkey);
			if (pcharacter == nullptr)
			{
				__debugbreak();
			}
			// 2. 섹터 결과 대입


			pcharacter->_lastRecvTime = GetTickCount64();

			// 제일 처음 생성된 플레이어인 경우 섹터 리스트 제외 건너뛰기
			if (pcharacter->_SectorX != 0xffff)
			{
				//2-a. 원래 있던 섹터에서 플레이어 삭제
				auto a = umapCharcterSector[pcharacter->_SectorY][pcharacter->_SectorX].find(pcharacter->_sessionkey.GetSessionId());
				if (a != umapCharcterSector[pcharacter->_SectorY][pcharacter->_SectorX].end())
				{
					umapCharcterSector[pcharacter->_SectorY][pcharacter->_SectorX].erase(a);
				}
			}

			//2-b. 새로운 섹터에 플레이어 대입
			INT64	AccountNo;
			*pPacket >> AccountNo;
			*pPacket >> pcharacter->_SectorX;
			*pPacket >> pcharacter->_SectorY;
			umapCharcterSector[pcharacter->_SectorY][pcharacter->_SectorX].emplace(pcharacter->_sessionkey.GetSessionId(), pcharacter);
			pPacket->SubRef();

			// 3.섹터 이동 결과 송신 패킷에 삽입
			CPacket* packetToSend = CPacket::Alloc();
			packetToSend->_MsgheaderSize = sizeof(PacketHeader);
			packetToSend->PutData((char*)&header, sizeof(PacketHeader));
			*packetToSend << (short)en_PACKET_SC_CHAT_RES_SECTOR_MOVE;
			*packetToSend << (INT64)pcharacter->_AccountNo;
			*packetToSend << (WORD)pcharacter->_SectorX;
			*packetToSend << (WORD)pcharacter->_SectorY;

			packetToSend->AddRef();
			bool ret = pServer->SendPacket(pcharacter->_sessionkey, packetToSend);
			packetToSend->SubRef();
			if (!ret)
			{
				printf("[Contents] SendPacket 실패, 세션 ID : %lld\n", pcharacter->_sessionkey.GetSessionId());
				//__debugbreak();
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
			Character* pcharacter = pServer->FindCharacter(sessionkey);
			if (pcharacter == nullptr)
			{
				__debugbreak();
			}

			pcharacter->_lastRecvTime = GetTickCount64();

			// 2. 메시지를 주위 섹터 플레이어에게 보내기
			INT64	AccountNo;
			WORD messageLen;
			char Message[500];
			*pPacket >> AccountNo;
			*pPacket >> messageLen;
			pPacket->GetData(Message, messageLen);

			CPacket* packetToSend = CPacket::Alloc();
			packetToSend->_MsgheaderSize = sizeof(PacketHeader);
			packetToSend->PutData((char*)&header, sizeof(PacketHeader));
			*packetToSend << (short)en_PACKET_SC_CHAT_RES_MESSAGE;
			packetToSend->PutData((char*)&pcharacter->_AccountNo, sizeof(pcharacter->_AccountNo));
			packetToSend->PutData((char*)pcharacter->_ID, sizeof(pcharacter->_ID));
			packetToSend->PutData((char*)pcharacter->_Nickname, sizeof(pcharacter->_Nickname));
			packetToSend->PutData((char*)&messageLen, sizeof(messageLen));
			packetToSend->PutData(Message, messageLen);
			pPacket->SubRef();

			int dx[9] = { -1,0,1,-1,0,1,-1,0,1 };
			int dy[9] = { -1,-1,-1,0,0,0,1,1,1 };
			packetToSend->AddRef();
			for (int i = 0; i < 9; i++)
			{
				int nx = pcharacter->_SectorX + dx[i];
				int ny = pcharacter->_SectorY + dy[i];
				if (nx < 0 || ny < 0 || nx >= 50 || ny >= 50) continue;
				for (auto& a : umapCharcterSector[ny][nx])
				{
					bool ret = pServer->SendPacket(a.second->_sessionkey, packetToSend);
					if (!ret)
					{
						//printf("[Contents] SendPacket 실패, 세션 ID : %lld\n", pcharacter->_sessionkey.GetSessionId());
						//__debugbreak();
					}
				}

			}
			packetToSend->SubRef();

			break;
		}
			

		//------------------------------------------------------------
		// 하트비트
		// 1.마지막 메시지 시간 갱신
		//------------------------------------------------------------	
		case en_PACKET_CS_CHAT_REQ_HEARTBEAT:
		{
			pPacket->SubRef();

			Character* pcharacter = pServer->FindCharacter(sessionkey);
			if (pcharacter == nullptr)
			{
				__debugbreak();
			}

			pcharacter->_lastRecvTime = GetTickCount64();
			break;
		}
		//-----------------------------------------------------------
		// 타이머 스레드가 
		// 플레이어 맵을 살핀 후 40초 이상 경과되어 플레이어 감지
		//-----------------------------------------------------------
		case en_PACKET_SS_TIMER_TICK:
		{
			pPacket->SubRef();

			uint64_t now = GetTickCount64();

			std::vector<Character*> expiredList;
			expiredList.reserve(128);

			// 1단계: 삭제 대상 수집
			for (auto& [key, ch] : umapCharacter)
			{
				if (now - ch->_lastRecvTime >= 40000)
				{
					expiredList.push_back(ch);
				}
			}

			// 2단계: 실제 삭제
			for (Character* ch : expiredList)
			{
				pServer->DeleteCharacter(ch->_sessionkey);
			}

			break;
		}
		//-----------------------------------------------------------
		// 네트워크 스레드가 알려주는 삭제된 세션 
		// 컨텐츠에서 세션을 삭제한다.
		//-----------------------------------------------------------
		case en_PACKET_SS_Session_Release:
		{
			pPacket->SubRef();
			pServer->DeleteCharacter(sessionkey);
			break;
		}
		case en_PACKET_SS_Create_Character:
		{
			pPacket->SubRef();
			pServer->CreateCharacter(sessionkey);
			//printf("[Contents] 세션 ID : %lld 캐릭터 생성\n", sessionkey.GetSessionId());
			break;
		}

		default:
			//말도 안되는 타입이 나왔다?
			//잘못된 패킷이 들어왔거나 인코딩 로직에 휴먼 에러
			__debugbreak();
			break;
		}
	}

	
	return 0;
}

//특정 주기마다 컨텐츠 스레드를 깨우는 메시지를 발생시키기 위한 스레드
//얘가 그냥 플레이어 죽었는지 체크하고 Flag세운 다음에 깨워도 되겠는데?
unsigned int __stdcall ChattingServer::TimerThread(LPVOID arg)
{
	ChattingServer* pServer = (ChattingServer*)arg;
	CPacket* ppacket = CPacket::Alloc();
	
	*ppacket << (short)en_PACKET_SS_TIMER_TICK;

	bool bWakeContentsThread = false;

	while (pServer->_bIsTimerThreadAlive)
	{
		//__debugbreak();
		Sleep(20000); // 20초 주기
		//ppacket->AddRef();

		//PostQueuedCompletionStatus(
		//	pServer->hContentCompletionPort,
		//	1,//byte
		//	-1,//id
		//	(LPOVERLAPPED)ppacket
		//);
	}
	return 0;
}


void Character::OnReuse()
{
	_AccountNo = -1;
	_SectorX = -1;
	_SectorY = -1;
	_sessionkey = {0};

	_lastRecvTime = 0;
}
