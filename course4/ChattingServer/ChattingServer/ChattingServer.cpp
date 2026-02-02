#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "ChattingServer.h"
#include "CPacketForMultiThread.h"
#include "CommonProtocol.h"
#include "MemoryPoolForLockFree.h"

#define SERVERPORT (6000)
#define BUFSIZE (1024 * 1024)
#define MSG_SIZE (8)

//채팅 서버에 로그인한 캐릭터를 저장해둔 맵
std::unordered_map<INT64, Character*> umapCharacter;

//캐릭터 리스트를 담아둔 섹터 맵
std::unordered_map<INT64, Character*> umapCharcterSector[50][50];

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

void ChattingServer::OnClientJoin(SOCKADDR_IN clientaddr, SessionID s)
{
    //printf("[Echo 서버] 클라이언트 접속 : IP 주소 = %s, 포트번호 = %d\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
}

void ChattingServer::OnClientLeave(SessionID s)
{
    //캐릭터 삭제같은걸 넣으면 될 것 같은데 에코에선 딱히 할게 없는 것으로 보임.
}

//컨텐츠 스레드 깨우기
void ChattingServer::OnRecv(SessionID sessionID, CPacket* pPacket)
{
	pPacket->AddRef();

	if (!PostQueuedCompletionStatus(hContentCompletionPort, pPacket->GetDataSize(), sessionID, (LPWSAOVERLAPPED)pPacket))
	{
		printf("[OnRecv] 컨텐츠 IOCP에 PQCS실패!\n");
		__debugbreak();
	}
}

void ChattingServer::OnError(int errorcode, char*)
{

}

//컨텐츠 스레드 함수
unsigned int __stdcall ChattingServer::ContentsThread(LPVOID arg)
{
	int retval;

	ChattingServer* pServer = (ChattingServer*)arg;

	bool bremoveDieCharacter = false;

	PacketHeader header;

	while (1) {
		//비동기 입출력 완료 기다리기
		DWORD cbTransferred;
		SOCKET client_sock;
		long long sessionId;
		CPacket* pPacket = nullptr;
		retval = GetQueuedCompletionStatus(pServer->hContentCompletionPort, &cbTransferred, (PULONG_PTR)&sessionId, (LPOVERLAPPED*)&pPacket, INFINITE);

		//비동기 입출력 결과 확인
		if (cbTransferred == 0 && sessionId == 0 && pPacket == nullptr)
		{
			//컨텐츠 스레드 종료
			//맵에 있는 모든 캐릭터 반환
			for (auto it = umapCharacter.begin(); it != umapCharacter.end(); )
			{
				Character* pcharacter = it->second;
				characterpool.Free(pcharacter);
				++it;
			}

			//모든 맵 정리
			umapCharacter.clear();
			for (int i = 0; i < 50; i++)
			{
				for (int j = 0; j < 50; j++)
				{
					umapCharcterSector[i][j].clear();
				}
			}

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

		CPacket* packetToSend = CPacket::Alloc();

		switch (type)
		{
		//------------------------------------------------------------
		// 채팅서버 로그인 요청
		// 1. 캐릭터 생성
		// 2. 캐릭터 맵에 추가
		// 3. 로그인 확인 메시지 전송
		//------------------------------------------------------------
		case en_PACKET_CS_CHAT_REQ_LOGIN:
		{
			// 1. 캐릭터 생성
			Character* pcharacter = characterpool.Alloc();
			pcharacter->OnReuse();

			*pPacket >> pcharacter->_AccountNo; 
			pPacket->GetData((char*)pcharacter->_ID, sizeof(pcharacter->_ID));
			pPacket->GetData((char*)pcharacter->_Nickname, sizeof(pcharacter->_Nickname));
			pPacket->GetData(pcharacter->_SessionKey, sizeof(pcharacter->_SessionKey));
			pcharacter->_sessionId = sessionId;

			
			BYTE	Status;			// 채팅서버 로그인 응답 0:실패	1:성공
			auto a = umapCharacter.find(pcharacter->_sessionId);

			// 2. 캐릭터 맵에 추가
			if (a != umapCharacter.end())
			{
				// 로그인을 했는데 이미 캐릭터가 있다?
				// 걍 말이 안되네..-> 세션 ID를 ++ 로 조회중, 아니지 삭제된 세션 ID를 새로 받긴 하잖슴.
				// 삭제된 세션 ID가 새롭게 부여되었고, 클라가 해당 세션 ID로 조회했는데 하트비트 이뤄지지않았으면?
				// 이러면 ID에 여전히 캐릭터가 있을 수 있는데??
				// 
				// 이 경우 남아있는 플레이어를 삭제하고 새로운 플레이어를 대입하는게 맞아보인다.
				// 
				// 게임 중간에 나갔다 바로 접속하면 게임 중이라고 뜨는 이유
				// 로그인 / 로그아웃에서 비동기로 DB에 접근함
				// 로그아웃이 되었다가 다시 로그인한게 회원 DB에 반영이 안된거면,
				// 게임 중이 아니라 이미 로그인 중이라고 뜨는게 맞는거고, 
				// 게임 중이라고 뜨는건 게임 서버 DB에 반영되지 않았다는거 아님?
				// 
				// 중복 로그인을 한다면? -> 애초에 로그아웃을 했다 = 세션의 삭제도 이루어졌다는 의미임.
				// 캐릭터도 삭제되야 함.
				// 새로운 로그인 접속
				umapCharacter[pcharacter->_sessionId] = pcharacter;
				Status = 1;
			}
			else
			{
				umapCharacter[pcharacter->_sessionId] = pcharacter;
				Status = 1;
			}

			INT64	AccountNo = pcharacter->_AccountNo;
			// 3. 
			packetToSend->_MsgheaderSize = sizeof(PacketHeader);
			packetToSend->PutData((char*)&header, sizeof(PacketHeader));
			*packetToSend << (short)en_PACKET_SC_CHAT_RES_LOGIN;
			*packetToSend << (BYTE)Status;
			*packetToSend << (INT64)pcharacter->_AccountNo;

			bool ret = pServer->SendPacket(pcharacter->_sessionId, packetToSend);
			if (!ret)
			{
				//SendPacket실패 세션이 이미 삭제된 경우
				pcharacter->_bDie = true;
				bremoveDieCharacter = true;
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
			INT64	AccountNo;
			*pPacket >> AccountNo;

			//1. 플레이어가 맵에 있는지 확인
			auto a = umapCharacter.find(sessionId);
			if (a != umapCharacter.end())
			{
				// 2. 섹터 결과 대입
				Character* pcharacter = umapCharacter[sessionId];
				pcharacter->_lastRecvTime = GetTickCount64();

				// 제일 처음 생성된 플레이어인 경우 섹터 리스트 제외 건너뛰기
				if (pcharacter->_SectorX != 0xffff)
				{
					//2-a. 원래 있던 섹터에서 플레이어 삭제
					auto a = umapCharcterSector[pcharacter->_SectorY][pcharacter->_SectorX].find(pcharacter->_AccountNo);
					if (a != umapCharcterSector[pcharacter->_SectorY][pcharacter->_SectorX].end())
					{
						umapCharcterSector[pcharacter->_SectorY][pcharacter->_SectorX].erase(a);
					}
				}
				//2-b. 새로운 섹터에 플레이어 대입
				*pPacket >> pcharacter->_SectorX;
				*pPacket >> pcharacter->_SectorY;
				umapCharcterSector[pcharacter->_SectorY][pcharacter->_SectorX].emplace(pcharacter->_AccountNo, pcharacter);

				// 3.섹터 이동 결과 송신 패킷에 삽입
				packetToSend->_MsgheaderSize = sizeof(PacketHeader);
				packetToSend->PutData((char*)&header, sizeof(PacketHeader));
				*packetToSend << (short)en_PACKET_SC_CHAT_RES_SECTOR_MOVE;
				*packetToSend << (INT64)pcharacter->_AccountNo;
				*packetToSend << (WORD)pcharacter->_SectorX;
				*packetToSend << (WORD)pcharacter->_SectorY;

				bool ret = pServer->SendPacket(pcharacter->_sessionId, packetToSend);
				if (!ret)
				{
					//SendPacket실패 세션이 이미 삭제된 경우
					pcharacter->_bDie = true;
					bremoveDieCharacter = true;
				}
		   }
		   else
		   {
				printf("[Contents] 맵에 없는 플레이어에 대해 섹터 이동 요청\n");
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
			*pPacket >> AccountNo;

			auto a = umapCharacter.find(sessionId);
			// 1. 계정을 플레이어 맵에서 확인
			if (a != umapCharacter.end())
			{

				Character* pcharacter = a->second;
				pcharacter->_lastRecvTime = GetTickCount64();

				// 2. 메시지를 주위 섹터 플레이어에게 보내기
				packetToSend->_MsgheaderSize = sizeof(PacketHeader);
				*packetToSend << (short)en_PACKET_SC_CHAT_RES_MESSAGE;
				packetToSend->PutData((char*)&pcharacter->_AccountNo, sizeof(pcharacter->_AccountNo));
				//pcharacter->_ID[19] = '\0';
				packetToSend->PutData((char*)pcharacter->_ID, sizeof(pcharacter->_ID));
				//pcharacter->_Nickname[19] = '\0';
				packetToSend->PutData((char*)pcharacter->_Nickname, sizeof(pcharacter->_Nickname));
				packetToSend->PutData(pPacket->GetBufferPtr(), pPacket->GetDataSize());

				int dx[9] = { -1,0,1,-1,0,1,-1,0,1 };
				int dy[9] = { -1,-1,-1,0,0,0,1,1,1 };
				for (int i = 0; i < 9; i++)
				{
					for (auto& a : umapCharcterSector[pcharacter->_SectorY + dy[i]][pcharacter->_SectorX + dx[i]])
					{
						if (!a.second->_bDie)
						{
							bool ret = pServer->SendPacket(a.second->_sessionId, packetToSend);
							if (!ret)
							{
								//SendPacket실패 세션이 이미 삭제된 경우
								pcharacter->_bDie = true;
								bremoveDieCharacter = true;
							}
						}
					}
					
				}
			}
			break;
		}
			

		//------------------------------------------------------------
		// 하트비트
		// 1.마지막 메시지 시간 갱신
		//------------------------------------------------------------	
		case en_PACKET_CS_CHAT_REQ_HEARTBEAT:
		{
			auto a = umapCharacter.find(sessionId);
			//1.
			if (a != umapCharacter.end())
			{
				a->second->_lastRecvTime = GetTickCount64();
				break;
			}
			else
			{
				printf("[Contents] 없는 플레이어를 대상으로 하트 비트 감지\n");
				__debugbreak();
			}
			break;
		}
		//-----------------------------------------------------------
		// 타이머 스레드가 
		// 플레이어 맵을 살핀 후 40초 이상 경과되어 플레이어 감지
		//-----------------------------------------------------------
		case en_PACKET_SS_TIMER_TICK:
		{
			bremoveDieCharacter = true;
			break;
		}

		default:
			break;
		}

		if (bremoveDieCharacter)
		{
			for (auto it = umapCharacter.begin(); it != umapCharacter.end(); )
			{
				Character* pcharacter = it->second;
				if (pcharacter->_bDie)
				{
					long long id = pcharacter->_sessionId;
					it = umapCharacter.erase(it);
					characterpool.Free(pcharacter);
					pServer->Disconnect(id);
				}
				else
				{
					++it;
				}
			}
			bremoveDieCharacter = false;
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
		Sleep(20000); // 20초 주기
		uint64_t now = GetTickCount64();

		for (auto& a : umapCharacter)
		{
			if (now - a.second->_lastRecvTime > 40000)
			{
				a.second->_bDie = true;
				bWakeContentsThread = true;
			}
		}

		if (bWakeContentsThread)
		{
			PostQueuedCompletionStatus(
				pServer->hContentCompletionPort,
				1,//byte
				-1,//id
				(LPOVERLAPPED)ppacket
			);
			bWakeContentsThread = false;
		}

	}

	ppacket->SubRef();

	return 0;
}


void Character::OnReuse()
{
	_AccountNo = -1;
	_SectorX = -1;
	_SectorY = -1;
	_sessionId = 0;

	_lastRecvTime = 0;
	_bDie = 0;
}
