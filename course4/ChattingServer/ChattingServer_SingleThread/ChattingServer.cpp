#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "ChattingServer.h"
#include "CPacketForMultiThread.h"
#include "CommonProtocol.h"
#include "MemoryPoolForLockFree.h"
#include <ctime>

//이거 중복 lan.cpp에서도 정의. 중복됨.
#define SERVERPORT (21501)
#define BUFSIZE (1024 * 16)
#define MSG_SIZE (8)

//------------------------------------------------------------
// 모니터링 데이터 타입 (MonitorProtocol.h 에서 발췌)
//------------------------------------------------------------
enum {
	dfMONITOR_DATA_TYPE_CHAT_SERVER_RUN		= 30,
	dfMONITOR_DATA_TYPE_CHAT_SERVER_CPU		= 31,
	dfMONITOR_DATA_TYPE_CHAT_SERVER_MEM		= 32,
	dfMONITOR_DATA_TYPE_CHAT_SESSION		= 33,
	dfMONITOR_DATA_TYPE_CHAT_PLAYER			= 34,
	dfMONITOR_DATA_TYPE_CHAT_UPDATE_TPS		= 35,
	dfMONITOR_DATA_TYPE_CHAT_PACKET_POOL	= 36,
	dfMONITOR_DATA_TYPE_CHAT_UPDATEMSG_POOL	= 37,
};

//채팅 서버에 로그인한 캐릭터를 보관해둘 맵
std::unordered_map<uint64_t, Character*> umapCharacter;

//캐릭터 리스트를 담아둘 섹터 맵
std::unordered_map<uint64_t, Character*> umapCharcterSector[50][50];

myMemorypool::CMemoryPool<Character> characterpool(10000,true);

ChattingServer::ChattingServer()
{
	//네트워크, 컨텐츠 스레드 사이의 완료 포트 생성
	hContentCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (hContentCompletionPort == NULL)
	{
		printf("[ChattingServer] 컨텐츠 스레드 IOCP 생성 실패");
		__debugbreak();
		return;
	}

	//컨텐츠 스레드 생성
	unsigned int uiThreadID;
	hContentThread = (HANDLE)_beginthreadex(
		NULL,
		0,
		ContentsThread,
		this,
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
			this,
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

		// PDH 초기화 (프로세스 CPU 사용률)
		PdhOpenQuery(NULL, 0, &_cpuQuery);
		PdhAddEnglishCounter(_cpuQuery, L"\\Process(ChattingServer_SingleThread)\\% Processor Time", 0, &_cpuCounter);
		PdhCollectQueryData(_cpuQuery);
}

ChattingServer::~ChattingServer()
{
	//컨텐츠 스레드 종료 신호
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

	//타이머 스레드 종료 신호
	_bIsTimerThreadAlive = false;

	//타이머 스레드 종료 대기
	WaitForSingleObject(hTimerThread, INFINITE);
	CloseHandle(hTimerThread);
}

//------------------------------------------------------------
// [추가] 모니터링 서버 접속
//------------------------------------------------------------
bool ChattingServer::ConnectMonitor(const char* monitorIP, int monitorPort, int serverNo)
{
	return _monitorClient.ConnectToMonitor(monitorIP, monitorPort, serverNo);
}

//----------------------
// 특정 대역의 IP를 차단하고 싶으면, 해당 IP와 매칭하면 block
// 특정 대역의 IP를 허용키고 싶으면, 해당 IP와 매칭하면 true
//----------------------
bool ChattingServer::OnConnectionRequest(std::string IP, int Port)
{
    return true;
}

void ChattingServer::OnClientJoin(SOCKADDR_IN clientaddr,SessionKey sessionkey)
{
	CPacket* pPacket = CPacket::Alloc();
	pPacket->AddRef();
	*pPacket << en_PACKET_SS_Create_Character;
	InterlockedIncrement(&_msgQueueSize);
	if (!PostQueuedCompletionStatus(hContentCompletionPort, pPacket->GetDataSize(), (ULONG_PTR)sessionkey.GetSessionKey(), (LPWSAOVERLAPPED)pPacket))
	{
		InterlockedDecrement(&_msgQueueSize);
		printf("[OnClientJoin] 컨텐츠 IOCP에 PQCS실패!\n");
		__debugbreak();
	}
}

void ChattingServer::OnClientLeave(SessionKey sessionkey)
{
	CPacket* pPacket = CPacket::Alloc();
	pPacket->AddRef();
	*pPacket << en_PACKET_SS_Session_Release;
	InterlockedIncrement(&_msgQueueSize);
	if (!PostQueuedCompletionStatus(hContentCompletionPort, pPacket->GetDataSize(), (ULONG_PTR)sessionkey.GetSessionKey(), (LPWSAOVERLAPPED)pPacket))
	{
		InterlockedDecrement(&_msgQueueSize);
		printf("[OnClientLeave] 컨텐츠 IOCP에 PQCS실패!\n");
		__debugbreak();
	}
}

//컨텐츠 스레드로 던지기
void ChattingServer::OnRecv(SessionKey sessionkey, CPacket* pPacket)
{
	pPacket->AddRef();

	InterlockedIncrement(&_msgQueueSize);
	if (!PostQueuedCompletionStatus(hContentCompletionPort, pPacket->GetDataSize(), (ULONG_PTR)sessionkey.GetSessionKey(), (LPWSAOVERLAPPED)pPacket))
	{
		InterlockedDecrement(&_msgQueueSize);
		printf("[OnRecv] 컨텐츠 IOCP에 PQCS실패!\n");
		__debugbreak();
	}
}

void ChattingServer::OnError(int errorcode, const char* msg)
{

}

Character* ChattingServer::FindCharacter(SessionKey sessionkey)
{
	auto a = umapCharacter.find(sessionkey.GetSessionId());
	if (a == umapCharacter.end())
	{
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

	// 3. 메모리 풀에 반환
	characterpool.Free(pcharacter);
	return true;
}

bool ChattingServer::CreateCharacter(SessionKey sessionkey)
{
	// 1. 캐릭터 생성
	Character* pcharacter = characterpool.Alloc();
	pcharacter->OnReuse();
	pcharacter->_sessionkey = sessionkey;
	pcharacter->_lastRecvTime = GetTickCount64();

	umapCharacter.emplace(sessionkey.GetSessionId(), pcharacter);
	return true;
}

//컨텐츠 스레드 함수
unsigned int __stdcall ChattingServer::ContentsThread(LPVOID arg)
{
	int retval;

	ChattingServer* pServer = (ChattingServer*)arg;

	PacketHeader header;

	while (1) {
		DWORD cbTransferred;
		SOCKET client_sock;

		ULONG_PTR completionKey = 0;

		CPacket* pPacket = nullptr;
		retval = GetQueuedCompletionStatus(pServer->hContentCompletionPort, &cbTransferred, &completionKey, (LPOVERLAPPED*)&pPacket, INFINITE);


		InterlockedDecrement(&pServer->_msgQueueSize);
		InterlockedIncrement(&pServer->_updateCount);

		//컨텐츠 스레드 종료
		if (cbTransferred == 0 && completionKey == 0 && pPacket == nullptr)
		{
			//모든 섹터 클리어
			for (int i = 0; i < 50; i++)
			{
				for (int j = 0; j < 50; j++)
				{
					umapCharcterSector[i][j].clear();
				}
			}

			//모든 캐릭터 삭제
			for (auto it = umapCharacter.begin(); it != umapCharacter.end(); )
			{
				Character* pcharacter = it->second;
				characterpool.Free(pcharacter);
				++it;
			}
			umapCharacter.clear();

			break;
		}
		else if (retval == 0)
		{
			while (1)
			{
				printf("[Contents] 비동기 함수가 호출되지 않고도 실패 할 수는 없는데\n");
			}
		}

		SessionKey sessionkey;
		sessionkey.value = completionKey;

		WORD type;
		*pPacket >> type;

		switch (type)
		{
		//------------------------------------------------------------
		// 채팅서버 로그인 요청
		//------------------------------------------------------------
		case en_PACKET_CS_CHAT_REQ_LOGIN:
		{
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

			BYTE	Status = 1;

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
			}
			break;
		}

		//------------------------------------------------------------
		// 채팅서버 섹터 이동 요청
		//------------------------------------------------------------
		case en_PACKET_CS_CHAT_REQ_SECTOR_MOVE:
		{
			Character* pcharacter = pServer->FindCharacter(sessionkey);
			if (pcharacter == nullptr)
			{
				__debugbreak();
			}

			pcharacter->_lastRecvTime = GetTickCount64();

			if (pcharacter->_SectorX != 0xffff)
			{
				auto a = umapCharcterSector[pcharacter->_SectorY][pcharacter->_SectorX].find(pcharacter->_sessionkey.GetSessionId());
				if (a != umapCharcterSector[pcharacter->_SectorY][pcharacter->_SectorX].end())
				{
					umapCharcterSector[pcharacter->_SectorY][pcharacter->_SectorX].erase(a);
				}
			}

			INT64	AccountNo;
			*pPacket >> AccountNo;
			*pPacket >> pcharacter->_SectorX;
			*pPacket >> pcharacter->_SectorY;
			umapCharcterSector[pcharacter->_SectorY][pcharacter->_SectorX].emplace(pcharacter->_sessionkey.GetSessionId(), pcharacter);
			pPacket->SubRef();

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
			}

			break;
		}
		//------------------------------------------------------------
		// 채팅서버 채팅보내기 요청
		//------------------------------------------------------------
		case en_PACKET_CS_CHAT_REQ_MESSAGE:
		{
			Character* pcharacter = pServer->FindCharacter(sessionkey);
			if (pcharacter == nullptr)
			{
				pPacket->SubRef();
				break;
			}

			pcharacter->_lastRecvTime = GetTickCount64();

			INT64 AccountNo;
			WORD messageLen;
			*pPacket >> AccountNo;
			*pPacket >> messageLen;

			if (messageLen > 500 || messageLen == 0)
			{
				pPacket->SubRef();
				pServer->Disconnect(pcharacter->_sessionkey);
				break;
			}

			char Message[500];
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
					}
				}

			}
			packetToSend->SubRef();

			break;
		}


		//------------------------------------------------------------
		// 하트비트
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
		// 플레이어 마지막 수신 후 40초 이상 경과되어 플레이어 킥
		//-----------------------------------------------------------
		case en_PACKET_SS_TIMER_TICK:
		{
			pPacket->SubRef();

			uint64_t now = GetTickCount64();

			std::vector<SessionKey> expiredList;
			expiredList.reserve(128);

			for (auto& [key, ch] : umapCharacter)
			{
				if (now - ch->_lastRecvTime >= 40000)
				{
					expiredList.push_back(ch->_sessionkey);
				}
			}

			for (SessionKey& sk : expiredList)
			{
				pServer->Disconnect(sk);
			}

			break;
		}
		//-----------------------------------------------------------
		// 네트워크 스레드가 알려주는 세션의 종료
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
			break;
		}

		default:
			__debugbreak();
			break;
		}
	}


	return 0;
}

//------------------------------------------------------------
// 타이머 스레드
// [변경] 1초 주기로 모니터링 데이터 전송 추가
//------------------------------------------------------------
unsigned int __stdcall ChattingServer::TimerThread(LPVOID arg)
{
	ChattingServer* pServer = (ChattingServer*)arg;

	int tickCount = 0;

	while (pServer->_bIsTimerThreadAlive)
	{
		Sleep(1000);
		tickCount++;

		//------------------------------------------------------------
		// [추가] 1초마다 모니터링 데이터 전송
		//------------------------------------------------------------
		if (pServer->_monitorClient.IsConnected())
		{
			int now = (int)time(NULL);

			// 채팅서버 동작 여부
			pServer->_monitorClient.SendMonitorData(dfMONITOR_DATA_TYPE_CHAT_SERVER_RUN, 1, now);

			// 채팅서버 CPU 사용률
			{
				PDH_FMT_COUNTERVALUE counterVal;
				PdhCollectQueryData(pServer->_cpuQuery);
				PdhGetFormattedCounterValue(pServer->_cpuCounter, PDH_FMT_LONG, NULL, &counterVal);
				pServer->_monitorClient.SendMonitorData(dfMONITOR_DATA_TYPE_CHAT_SERVER_CPU, (int)counterVal.longValue, now);
			}

			// 채팅서버 메모리 사용량 (MByte)
			{
				PROCESS_MEMORY_COUNTERS pmc;
				GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));
				pServer->_monitorClient.SendMonitorData(dfMONITOR_DATA_TYPE_CHAT_SERVER_MEM, (int)(pmc.WorkingSetSize / (1024 * 1024)), now);
			}

			// 채팅서버 세션 수
			pServer->_monitorClient.SendMonitorData(dfMONITOR_DATA_TYPE_CHAT_SESSION, pServer->GetSessionCount(), now);

			// 채팅서버 인증성공 플레이어 수
			pServer->_monitorClient.SendMonitorData(dfMONITOR_DATA_TYPE_CHAT_PLAYER, (int)umapCharacter.size(), now);

			// 채팅서버 UPDATE TPS
			{
				int tps = InterlockedExchange(&pServer->_updateCount, 0);
				pServer->_monitorClient.SendMonitorData(dfMONITOR_DATA_TYPE_CHAT_UPDATE_TPS, tps, now);
			}

			// 채팅서버 패킷풀 사용량
			{
				int useCount = (int)CPacket::packetPool.GetUseCount();
				if (useCount < 0) useCount = 0;
				pServer->_monitorClient.SendMonitorData(dfMONITOR_DATA_TYPE_CHAT_PACKET_POOL, useCount, now);
			}

			// 채팅서버 UPDATE MSG 큐 사이즈
			{
				int queueSize = pServer->_msgQueueSize;
				if (queueSize < 0) queueSize = 0;
				pServer->_monitorClient.SendMonitorData(dfMONITOR_DATA_TYPE_CHAT_UPDATEMSG_POOL, queueSize, now);
			}
		}

		//------------------------------------------------------------
		// 20초마다 하트비트 타이머 틱
		//------------------------------------------------------------
		if (tickCount >= 20)
		{
			tickCount = 0;

			CPacket* ppacket = CPacket::Alloc();
			ppacket->AddRef();
			*ppacket << (short)en_PACKET_SS_TIMER_TICK;

			PostQueuedCompletionStatus(
				pServer->hContentCompletionPort,
				1,
				(ULONG_PTR)-1,
				(LPOVERLAPPED)ppacket
			);
		}
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
