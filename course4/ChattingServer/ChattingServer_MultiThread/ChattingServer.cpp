#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "ChattingServer.h"
#include "CPacketForMultiThread.h"
#include "CommonProtocol.h"
#include "MemoryPoolForLockFree.h"
#include "CSystemLog.h"
#include <ctime>

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

myMemorypool::CMemoryPool<Character> characterpool(20000, true);

ChattingServer::ChattingServer()
{
	//------------------------------------------------------------
	// [MultiThread] SRWLock 초기화
	//------------------------------------------------------------
	InitializeSRWLock(&_characterLock);

	// unordered_map 사전 예약 (rehash 방지, 메모리 급증 방지)
	_umapCharacter.reserve(20000);
	_umapAccountSession.reserve(20000);

	// 타이머 스레드 생성
	unsigned int uiThreadID;
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
	PdhAddEnglishCounter(_cpuQuery, L"\\Process(ChattingServer_MultiThread)\\% Processor Time", 0, &_cpuCounter);
	PdhCollectQueryData(_cpuQuery);
}

ChattingServer::~ChattingServer()
{
	// 모니터링 클라이언트 먼저 종료 (워커 스레드 정리 → purecall 방지)
	_monitorClient.Stop();

	// 타이머 스레드 종료 신호
	_bIsTimerThreadAlive = false;

	// 타이머 스레드 종료 대기
	WaitForSingleObject(hTimerThread, INFINITE);
	CloseHandle(hTimerThread);

	// 모든 캐릭터 정리
	AcquireSRWLockExclusive(&_characterLock);
	for (int i = 0; i < 50; i++)
	{
		for (int j = 0; j < 50; j++)
		{
			_umapCharacterSector[i][j].clear();
		}
	}

	for (auto& [key, ch] : _umapCharacter)
	{
		characterpool.Free(ch);
	}
	_umapCharacter.clear();
	_umapAccountSession.clear();
	ReleaseSRWLockExclusive(&_characterLock);
}

//------------------------------------------------------------
// [추가] 모니터링 서버 접속
//------------------------------------------------------------
bool ChattingServer::ConnectMonitor(const char* monitorIP, int monitorPort, int serverNo)
{
	return _monitorClient.ConnectToMonitor(monitorIP, monitorPort, serverNo);
}

bool ChattingServer::OnConnectionRequest(std::string IP, int Port)
{
    return true;
}

//------------------------------------------------------------
// [MultiThread] 워커/Accept 스레드에서 직접 호출
// Exclusive Lock으로 캐릭터 생성
//------------------------------------------------------------
void ChattingServer::OnClientJoin(SOCKADDR_IN clientaddr, SessionKey sessionkey)
{
	InterlockedIncrement(&_activeInJoin);
	CreateCharacter(sessionkey);
	InterlockedDecrement(&_activeInJoin);
}

//------------------------------------------------------------
// [MultiThread] 워커 스레드에서 직접 호출
// Exclusive Lock으로 캐릭터 삭제
//------------------------------------------------------------
void ChattingServer::OnClientLeave(SessionKey sessionkey)
{
	InterlockedIncrement(&_activeInLeave);
	DeleteCharacter(sessionkey);
	InterlockedDecrement(&_activeInLeave);
}

//------------------------------------------------------------
// [MultiThread] 워커 스레드에서 직접 채팅 로직 처리
// PQCS를 통한 ContentsThread 경유 없이 바로 처리
//------------------------------------------------------------
void ChattingServer::OnRecv(SessionKey sessionkey, CPacket* pPacket)
{
	InterlockedIncrement(&_updateCount);

	WORD type;
	*pPacket >> type;

	switch (type)
	{
	case en_PACKET_CS_CHAT_REQ_LOGIN:
		Handle_CS_CHAT_REQ_LOGIN(sessionkey, pPacket);
		break;

	case en_PACKET_CS_CHAT_REQ_SECTOR_MOVE:
		Handle_CS_CHAT_REQ_SECTOR_MOVE(sessionkey, pPacket);
		break;

	case en_PACKET_CS_CHAT_REQ_MESSAGE:
		Handle_CS_CHAT_REQ_MESSAGE(sessionkey, pPacket);
		break;

	case en_PACKET_CS_CHAT_REQ_HEARTBEAT:
		Handle_CS_CHAT_REQ_HEARTBEAT(sessionkey, pPacket);
		break;

	default:
		LOG(L"ChattingServer", CSystemLog::LEVEL_ERROR,
			L"[Session:%llu] Unknown packet type: %d - Disconnect",
			sessionkey.GetSessionId(), type);
		Disconnect(sessionkey);
		break;
	}
}

void ChattingServer::OnError(int errorcode, const char* msg)
{

}

//------------------------------------------------------------
// 캐릭터 조회 (호출자가 락을 보유해야 함)
//------------------------------------------------------------
Character* ChattingServer::FindCharacter(SessionKey sessionkey)
{
	auto a = _umapCharacter.find(sessionkey.GetSessionId());
	if (a == _umapCharacter.end())
	{
		return nullptr;
	}

	return a->second;
}

//------------------------------------------------------------
// 캐릭터 삭제 (Exclusive Lock 내부에서 획득)
//------------------------------------------------------------
bool ChattingServer::DeleteCharacter(SessionKey sessionkey)
{
	AcquireSRWLockExclusive(&_characterLock);

	Character* pcharacter = FindCharacter(sessionkey);
	if (pcharacter == nullptr)
	{
		ReleaseSRWLockExclusive(&_characterLock);
		return false;
	}

	// 1. 섹터 맵에서 제거
	if (pcharacter->_SectorY != 0xffff)
	{
		auto& sectorMap = _umapCharacterSector[pcharacter->_SectorY][pcharacter->_SectorX];
		sectorMap.erase(pcharacter->_sessionkey.GetSessionId());
	}

	// 2. AccountNo 맵에서 제거 (로그인 완료된 캐릭터만)
	if (pcharacter->_AccountNo != -1)
	{
		auto itAccount = _umapAccountSession.find(pcharacter->_AccountNo);
		// 세션키가 일치하는 경우에만 제거 (다른 세션이 같은 AccountNo로 등록된 경우 방지)
		if (itAccount != _umapAccountSession.end() &&
			itAccount->second.GetSessionId() == pcharacter->_sessionkey.GetSessionId())
		{
			_umapAccountSession.erase(itAccount);
		}
	}

	// 3. 전체 캐릭터 맵에서 제거
	_umapCharacter.erase(pcharacter->_sessionkey.GetSessionId());

	ReleaseSRWLockExclusive(&_characterLock);

	// 3. 메모리 풀에 반환 (락 밖에서 해도 안전)
	characterpool.Free(pcharacter);
	return true;
}

//------------------------------------------------------------
// 캐릭터 생성 (Exclusive Lock 내부에서 획득)
//------------------------------------------------------------
bool ChattingServer::CreateCharacter(SessionKey sessionkey)
{
	Character* pcharacter = characterpool.Alloc();
	pcharacter->OnReuse();
	pcharacter->_sessionkey = sessionkey;
	pcharacter->_lastRecvTime = GetTickCount64();

	AcquireSRWLockExclusive(&_characterLock);
	_umapCharacter.emplace(sessionkey.GetSessionId(), pcharacter);
	ReleaseSRWLockExclusive(&_characterLock);

	return true;
}

//------------------------------------------------------------
// 로그인 요청 처리 (Exclusive Lock - 중복 로그인 감지를 위해)
// 정책: "kick old, accept new" - 이미 로그인된 AccountNo면 기존 세션 끊기
//------------------------------------------------------------
void ChattingServer::Handle_CS_CHAT_REQ_LOGIN(SessionKey sessionkey, CPacket* pPacket)
{
	InterlockedIncrement(&_activeInLogin);
	PacketHeader header;

	AcquireSRWLockExclusive(&_characterLock);

	Character* pcharacter = FindCharacter(sessionkey);
	if (pcharacter == nullptr)
	{
		ReleaseSRWLockExclusive(&_characterLock);
		InterlockedDecrement(&_activeInLogin);
		return;
	}

	// 패킷 크기 검증: Login = AccountNo(8) + ID(40) + Nickname(40) + Token(64) = 152
	int expectedSize = sizeof(INT64) + sizeof(WCHAR) * 20 + sizeof(WCHAR) * 20 + 64;
	if (pPacket->GetDataSize() != expectedSize)
	{
		LOG(L"ChattingServer", CSystemLog::LEVEL_ERROR,
			L"[Session:%llu] Login packet size mismatch (expected:%d, actual:%d) - Disconnect",
			pcharacter->_sessionkey.GetSessionId(), expectedSize, pPacket->GetDataSize());
		SessionKey charKey = pcharacter->_sessionkey;
		ReleaseSRWLockExclusive(&_characterLock);
		Disconnect(charKey);
		InterlockedDecrement(&_activeInLogin);
		return;
	}

	*pPacket >> pcharacter->_AccountNo;
	pPacket->GetData((char*)pcharacter->_ID, sizeof(pcharacter->_ID));
	pPacket->GetData((char*)pcharacter->_Nickname, sizeof(pcharacter->_Nickname));
	pPacket->GetData(pcharacter->_Token, sizeof(pcharacter->_Token));

	SessionKey charKey = pcharacter->_sessionkey;
	INT64 accountNo = pcharacter->_AccountNo;

	//------------------------------------------------------------
	// 중복 로그인 감지: 같은 AccountNo가 이미 로그인되어 있으면
	// 기존 세션을 끊고, 새 세션을 받아들임
	//------------------------------------------------------------
	SessionKey oldSessionKey = {};
	auto itAccount = _umapAccountSession.find(accountNo);
	if (itAccount != _umapAccountSession.end())
	{
		oldSessionKey = itAccount->second;
		LOG(L"ChattingServer", CSystemLog::LEVEL_ERROR,
			L"[Session:%llu] Duplicate login - kick old session:%llu (AccountNo:%lld)",
			charKey.GetSessionId(), oldSessionKey.GetSessionId(), accountNo);

		// 맵에서 기존 세션 키를 새 세션으로 덮어쓰기
		itAccount->second = charKey;
	}
	else
	{
		_umapAccountSession.emplace(accountNo, charKey);
	}

	ReleaseSRWLockExclusive(&_characterLock);

	// 기존 세션 끊기 (락 밖에서 — Disconnect → OnClientLeave → Exclusive Lock 데드락 방지)
	if (oldSessionKey.GetSessionId() != 0)
	{
		Disconnect(oldSessionKey);
	}

	BYTE Status = 1;

	CPacket* packetToSend = CPacket::Alloc();
	packetToSend->_MsgheaderSize = sizeof(PacketHeader);
	packetToSend->PutData((char*)&header, sizeof(PacketHeader));
	*packetToSend << (short)en_PACKET_SC_CHAT_RES_LOGIN;
	*packetToSend << (BYTE)Status;
	*packetToSend << (INT64)accountNo;

	SendPacket(charKey, packetToSend);
	packetToSend->SubRef();
	InterlockedDecrement(&_activeInLogin);
}

//------------------------------------------------------------
// 섹터 이동 요청 처리 (Exclusive Lock)
//------------------------------------------------------------
void ChattingServer::Handle_CS_CHAT_REQ_SECTOR_MOVE(SessionKey sessionkey, CPacket* pPacket)
{
	InterlockedIncrement(&_activeInSectorMove);
	PacketHeader header;

	AcquireSRWLockExclusive(&_characterLock);

	Character* pcharacter = FindCharacter(sessionkey);
	if (pcharacter == nullptr)
	{
		ReleaseSRWLockExclusive(&_characterLock);
		InterlockedDecrement(&_activeInSectorMove);
		return;
	}

	pcharacter->_lastRecvTime = GetTickCount64();

	// 이전 섹터에서 제거
	if (pcharacter->_SectorX != 0xffff)
	{
		auto a = _umapCharacterSector[pcharacter->_SectorY][pcharacter->_SectorX].find(pcharacter->_sessionkey.GetSessionId());
		if (a != _umapCharacterSector[pcharacter->_SectorY][pcharacter->_SectorX].end())
		{
			_umapCharacterSector[pcharacter->_SectorY][pcharacter->_SectorX].erase(a);
		}
	}

	// 패킷 크기 검증: SectorMove = AccountNo(8) + SectorX(2) + SectorY(2) = 12
	int expectedSize = sizeof(INT64) + sizeof(WORD) + sizeof(WORD);
	if (pPacket->GetDataSize() != expectedSize)
	{
		LOG(L"ChattingServer", CSystemLog::LEVEL_ERROR,
			L"[Session:%llu] SectorMove packet size mismatch (expected:%d, actual:%d) - Disconnect",
			pcharacter->_sessionkey.GetSessionId(), expectedSize, pPacket->GetDataSize());
		SessionKey charKey = pcharacter->_sessionkey;
		ReleaseSRWLockExclusive(&_characterLock);
		Disconnect(charKey);
		InterlockedDecrement(&_activeInSectorMove);
		return;
	}

	INT64 AccountNo;
	WORD newSectorX, newSectorY;
	*pPacket >> AccountNo;
	*pPacket >> newSectorX;
	*pPacket >> newSectorY;

	if (newSectorX >= 50 || newSectorY >= 50)
	{
		LOG(L"ChattingServer", CSystemLog::LEVEL_ERROR,
			L"[Session:%llu] Invalid Sector (%d, %d) - Disconnect",
			pcharacter->_sessionkey.GetSessionId(), newSectorX, newSectorY);
		SessionKey charKey = pcharacter->_sessionkey;
		ReleaseSRWLockExclusive(&_characterLock);
		Disconnect(charKey);
		InterlockedDecrement(&_activeInSectorMove);
		return;
	}

	pcharacter->_SectorX = newSectorX;
	pcharacter->_SectorY = newSectorY;
	_umapCharacterSector[pcharacter->_SectorY][pcharacter->_SectorX].emplace(pcharacter->_sessionkey.GetSessionId(), pcharacter);

	// 응답에 필요한 데이터 복사
	SessionKey charKey = pcharacter->_sessionkey;
	INT64 charAccountNo = pcharacter->_AccountNo;
	WORD sectorX = pcharacter->_SectorX;
	WORD sectorY = pcharacter->_SectorY;

	ReleaseSRWLockExclusive(&_characterLock);

	CPacket* packetToSend = CPacket::Alloc();
	packetToSend->_MsgheaderSize = sizeof(PacketHeader);
	packetToSend->PutData((char*)&header, sizeof(PacketHeader));
	*packetToSend << (short)en_PACKET_SC_CHAT_RES_SECTOR_MOVE;
	*packetToSend << (INT64)charAccountNo;
	*packetToSend << (WORD)sectorX;
	*packetToSend << (WORD)sectorY;

	SendPacket(charKey, packetToSend);
	packetToSend->SubRef();
	InterlockedDecrement(&_activeInSectorMove);
}

//------------------------------------------------------------
// 채팅 메시지 처리 (Shared Lock - 읽기만 필요)
//------------------------------------------------------------
void ChattingServer::Handle_CS_CHAT_REQ_MESSAGE(SessionKey sessionkey, CPacket* pPacket)
{
	PacketHeader header;

	InterlockedIncrement(&_activeInChatMsg);
	//------------------------------------------------------------
	// [데드락 수정] Shared lock 안에서는 데이터 복사 + 대상 수집만
	// SendPacket은 lock 밖에서 호출 (SendPacket → DecreaseSessionIO
	// → OnClientLeave → DeleteCharacter → Exclusive lock = 데드락)
	//------------------------------------------------------------

	INT64 charAccountNo;
	WCHAR charID[20];
	WCHAR charNickname[20];
	WORD messageLen;
	char Message[500];
	std::vector<SessionKey> targets;
	targets.reserve(500);

	AcquireSRWLockShared(&_characterLock);

	Character* pcharacter = FindCharacter(sessionkey);
	if (pcharacter == nullptr)
	{
		ReleaseSRWLockShared(&_characterLock);
		InterlockedDecrement(&_activeInChatMsg);
		return;
	}

	pcharacter->_lastRecvTime = GetTickCount64();

	INT64 AccountNo;
	*pPacket >> AccountNo;
	*pPacket >> messageLen;

	// 검증 1: MessageLen 범위 체크
	if (messageLen > 500 || messageLen == 0)
	{
		LOG(L"ChattingServer", CSystemLog::LEVEL_ERROR,
			L"[Session:%llu] Invalid MessageLen: %d - Disconnect",
			pcharacter->_sessionkey.GetSessionId(), messageLen);
		SessionKey charKey = pcharacter->_sessionkey;
		ReleaseSRWLockShared(&_characterLock);
		Disconnect(charKey);
		InterlockedDecrement(&_activeInChatMsg);
		return;
	}

	// 검증 2: 패킷 잔여 크기와 MessageLen 교차 검증
	// 헤더(Type 2 + AccountNo 8 + MessageLen 2) 이후 남은 데이터 = MessageLen이어야 함
	int remainSize = pPacket->GetDataSize();
	if (remainSize != messageLen)
	{
		LOG(L"ChattingServer", CSystemLog::LEVEL_ERROR,
			L"[Session:%llu] MessageLen mismatch (MessageLen:%d, Remain:%d) - Disconnect",
			pcharacter->_sessionkey.GetSessionId(), messageLen, remainSize);
		SessionKey charKey = pcharacter->_sessionkey;
		ReleaseSRWLockShared(&_characterLock);
		Disconnect(charKey);
		InterlockedDecrement(&_activeInChatMsg);
		return;
	}

	pPacket->GetData(Message, messageLen);

	// 패킷 생성에 필요한 데이터 복사
	charAccountNo = pcharacter->_AccountNo;
	memcpy(charID, pcharacter->_ID, sizeof(charID));
	memcpy(charNickname, pcharacter->_Nickname, sizeof(charNickname));

	// 주변 9섹터 대상 세션 수집
	int dx[9] = { -1,0,1,-1,0,1,-1,0,1 };
	int dy[9] = { -1,-1,-1,0,0,0,1,1,1 };
	for (int i = 0; i < 9; i++)
	{
		int nx = pcharacter->_SectorX + dx[i];
		int ny = pcharacter->_SectorY + dy[i];
		if (nx < 0 || ny < 0 || nx >= 50 || ny >= 50) continue;
		for (auto& a : _umapCharacterSector[ny][nx])
		{
			targets.push_back(a.second->_sessionkey);
		}
	}

	ReleaseSRWLockShared(&_characterLock);

	// 브로드캐스트 패킷 생성 (lock 밖)
	CPacket* packetToSend = CPacket::Alloc();
	packetToSend->_MsgheaderSize = sizeof(PacketHeader);
	packetToSend->PutData((char*)&header, sizeof(PacketHeader));
	*packetToSend << (short)en_PACKET_SC_CHAT_RES_MESSAGE;
	packetToSend->PutData((char*)&charAccountNo, sizeof(charAccountNo));
	packetToSend->PutData((char*)charID, sizeof(charID));
	packetToSend->PutData((char*)charNickname, sizeof(charNickname));
	packetToSend->PutData((char*)&messageLen, sizeof(messageLen));
	packetToSend->PutData(Message, messageLen);

	// SendPacket은 lock 밖에서 호출 (데드락 방지)
	for (auto& sk : targets)
	{
		SendPacket(sk, packetToSend);
	}
	packetToSend->SubRef();
	InterlockedDecrement(&_activeInChatMsg);
}

//------------------------------------------------------------
// 하트비트 처리 (Shared Lock)
//------------------------------------------------------------
void ChattingServer::Handle_CS_CHAT_REQ_HEARTBEAT(SessionKey sessionkey, CPacket* pPacket)
{
	InterlockedIncrement(&_activeInHeartbeat);
	AcquireSRWLockShared(&_characterLock);

	Character* pcharacter = FindCharacter(sessionkey);
	if (pcharacter != nullptr)
	{
		pcharacter->_lastRecvTime = GetTickCount64();
	}

	ReleaseSRWLockShared(&_characterLock);
	InterlockedDecrement(&_activeInHeartbeat);
}

//------------------------------------------------------------
// 타이머 스레드
// [MultiThread] 하트비트 체크를 직접 수행 (Shared Lock)
// 1초 주기로 모니터링 데이터 전송 + 화면 갱신
//------------------------------------------------------------
unsigned int __stdcall ChattingServer::TimerThread(LPVOID arg)
{
	ChattingServer* pServer = (ChattingServer*)arg;

	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

	// 초기 printf 잔상 제거: 콘솔 전체 클리어
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(hConsole, &csbi);
	DWORD consoleSize = csbi.dwSize.X * csbi.dwSize.Y;
	DWORD charsWritten;
	COORD topLeft = { 0, 0 };
	FillConsoleOutputCharacterA(hConsole, ' ', consoleSize, topLeft, &charsWritten);
	FillConsoleOutputAttribute(hConsole, csbi.wAttributes, consoleSize, topLeft, &charsWritten);
	SetConsoleCursorPosition(hConsole, topLeft);

	// 커서 숨기기
	CONSOLE_CURSOR_INFO cursorInfo;
	GetConsoleCursorInfo(hConsole, &cursorInfo);
	cursorInfo.bVisible = FALSE;
	SetConsoleCursorInfo(hConsole, &cursorInfo);

	const int LINE_WIDTH = 70;
	char buf[8192];

	int tickCount = 0;

	while (pServer->_bIsTimerThreadAlive)
	{
		Sleep(1000);
		tickCount++;

		//------------------------------------------------------------
		// 1초마다 모니터링 데이터 수집
		//------------------------------------------------------------
		int cpuVal = 0;
		int memMB = 0;
		int sessionCount = pServer->GetSessionCount();
		int playerCount = 0;
		long long sectorBucketTotal = 0;
		int updateTPS = InterlockedExchange(&pServer->_updateCount, 0);
		int packetPoolUse = (int)CPacket::packetPool.GetUseCount();
		if (packetPoolUse < 0) packetPoolUse = 0;
		int acceptTPS = pServer->getAcceptTPS();
		bool monConnected = pServer->_monitorClient.IsConnected();
		int monSendCount = 0;

		// CPU (전체 코어 대비 프로세스 사용률)
		// PDH % Processor Time: 코어1개=100%, 4코어 풀사용=400%
		// 코어 수로 나눠 전체 CPU 대비 비율로 변환
		{
			PDH_FMT_COUNTERVALUE counterVal;
			PdhCollectQueryData(pServer->_cpuQuery);
			PdhGetFormattedCounterValue(pServer->_cpuCounter, PDH_FMT_DOUBLE, NULL, &counterVal);
			SYSTEM_INFO si;
			GetSystemInfo(&si);
			/*cpuVal = (int)(counterVal.doubleValue / si.dwNumberOfProcessors);*/
			cpuVal = pServer->_cpuUsage.Update();
		}

		// 메모리
		{
			PROCESS_MEMORY_COUNTERS pmc;
			GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));
			memMB = (int)(pmc.WorkingSetSize / (1024 * 1024));
		}

		// 플레이어 수
		{
			InterlockedIncrement(&pServer->_activeInTimerLock);
			AcquireSRWLockShared(&pServer->_characterLock);
			playerCount = (int)pServer->_umapCharacter.size();
			sectorBucketTotal = 0;
			for (int sy = 0; sy < 50; sy++)
				for (int sx = 0; sx < 50; sx++)
					sectorBucketTotal += pServer->_umapCharacterSector[sy][sx].bucket_count();
			ReleaseSRWLockShared(&pServer->_characterLock);
			InterlockedDecrement(&pServer->_activeInTimerLock);
		}

		//------------------------------------------------------------
		// 모니터링 서버에 데이터 전송
		//------------------------------------------------------------
		if (monConnected)
		{
			int now = (int)time(NULL);

			pServer->_monitorClient.SendMonitorData(dfMONITOR_DATA_TYPE_CHAT_SERVER_RUN, 1, now);
			pServer->_monitorClient.SendMonitorData(dfMONITOR_DATA_TYPE_CHAT_SERVER_CPU, cpuVal, now);
			pServer->_monitorClient.SendMonitorData(dfMONITOR_DATA_TYPE_CHAT_SERVER_MEM, memMB, now);
			pServer->_monitorClient.SendMonitorData(dfMONITOR_DATA_TYPE_CHAT_SESSION, sessionCount, now);
			pServer->_monitorClient.SendMonitorData(dfMONITOR_DATA_TYPE_CHAT_PLAYER, playerCount, now);
			pServer->_monitorClient.SendMonitorData(dfMONITOR_DATA_TYPE_CHAT_UPDATE_TPS, updateTPS, now);
			pServer->_monitorClient.SendMonitorData(dfMONITOR_DATA_TYPE_CHAT_PACKET_POOL, packetPoolUse, now);
			pServer->_monitorClient.SendMonitorData(dfMONITOR_DATA_TYPE_CHAT_UPDATEMSG_POOL, 0, now);
			monSendCount = 8;
		}

		//------------------------------------------------------------
		// 디스플레이 값 저장
		//------------------------------------------------------------
		pServer->_dispCpu = cpuVal;
		pServer->_dispMemMB = memMB;
		pServer->_dispSessionCount = sessionCount;
		pServer->_dispPlayerCount = playerCount;
		pServer->_dispUpdateTPS = updateTPS;
		pServer->_dispPacketPoolUse = packetPoolUse;
		pServer->_dispAcceptTPS = acceptTPS;
		pServer->_dispMonitorSendCount = monSendCount;
		pServer->_dispMonitorConnected = monConnected;

		//------------------------------------------------------------
		// 화면 갱신 (더블 버퍼 방식)
		//------------------------------------------------------------
		{
			int pos = 0;
			char line[128];

			pos += sprintf_s(buf + pos, sizeof(buf) - pos,
				"%-*s\n", LINE_WIDTH,
				"=== ChattingServer MultiThread ===");
			pos += sprintf_s(buf + pos, sizeof(buf) - pos,
				"%-*s\n", LINE_WIDTH,
				"----------------------------------------------------------------------");

			// 모니터링 연결 상태
			sprintf_s(line, sizeof(line),
				"  Monitor: %s   |   Send/s: %d",
				monConnected ? "Connected" : "Disconnected",
				monSendCount);
			pos += sprintf_s(buf + pos, sizeof(buf) - pos, "%-*s\n", LINE_WIDTH, line);

			pos += sprintf_s(buf + pos, sizeof(buf) - pos,
				"%-*s\n", LINE_WIDTH,
				"----------------------------------------------------------------------");

			// CPU / 메모리
			sprintf_s(line, sizeof(line),
				"  CPU: %d%%       Memory: %d MB",
				cpuVal, memMB);
			pos += sprintf_s(buf + pos, sizeof(buf) - pos, "%-*s\n", LINE_WIDTH, line);

			// 세션
			sprintf_s(line, sizeof(line),
				"  Session: %d", sessionCount);
			pos += sprintf_s(buf + pos, sizeof(buf) - pos, "%-*s\n", LINE_WIDTH, line);

			pos += sprintf_s(buf + pos, sizeof(buf) - pos,
				"%-*s\n", LINE_WIDTH,
				"----------------------------------------------------------------------");

			// TPS
			long long totalAccept = pServer->getTotalAcceptCount();
			sprintf_s(line, sizeof(line),
				"  Update TPS: %d    Accept TPS: %d    Total Accept: %lld",
				updateTPS, acceptTPS, totalAccept);
			pos += sprintf_s(buf + pos, sizeof(buf) - pos, "%-*s\n", LINE_WIDTH, line);

			// 동적 리소스 (풀 사용량 / 전체 할당량)
			int packetPoolTotal = (int)CPacket::packetPool.GetCapacityCount();
			int charPoolUse = (int)characterpool.GetUseCount();
			int charPoolTotal = (int)characterpool.GetCapacityCount();

			sprintf_s(line, sizeof(line),
				"  PacketPool: %d/%d   CharPool: %d/%d",
				packetPoolUse, packetPoolTotal, charPoolUse, charPoolTotal);
			pos += sprintf_s(buf + pos, sizeof(buf) - pos, "%-*s\n", LINE_WIDTH, line);

			long sendBufFullDisconnect = pServer->getSendBufferFullCount();
			long heartbeatTimeout = pServer->_heartbeatTimeoutCount;
			sprintf_s(line, sizeof(line),
				"  SendBufFull: %ld    HeartbeatTimeout: %ld",
				sendBufFullDisconnect, heartbeatTimeout);
			pos += sprintf_s(buf + pos, sizeof(buf) - pos, "%-*s\n", LINE_WIDTH, line);

			pos += sprintf_s(buf + pos, sizeof(buf) - pos,
				"%-*s\n", LINE_WIDTH,
				"----------------------------------------------------------------------");

			// 스레드 활동 (진단용)
			pos += sprintf_s(buf + pos, sizeof(buf) - pos,
				"%-*s\n", LINE_WIDTH,
				"  [Thread Activity]");

			sprintf_s(line, sizeof(line),
				"    Join:%ld  Leave:%ld  Login:%ld  SectorMove:%ld",
				pServer->_activeInJoin, pServer->_activeInLeave,
				pServer->_activeInLogin, pServer->_activeInSectorMove);
			pos += sprintf_s(buf + pos, sizeof(buf) - pos, "%-*s\n", LINE_WIDTH, line);

			sprintf_s(line, sizeof(line),
				"    ChatMsg:%ld  Heartbeat:%ld  TimerLock:%ld",
				pServer->_activeInChatMsg, pServer->_activeInHeartbeat,
				pServer->_activeInTimerLock);
			pos += sprintf_s(buf + pos, sizeof(buf) - pos, "%-*s\n", LINE_WIDTH, line);

			pos += sprintf_s(buf + pos, sizeof(buf) - pos,
				"%-*s\n", LINE_WIDTH,
				"----------------------------------------------------------------------");

			// 빈 줄 (잔상 제거)
			for (int i = 0; i < 3; i++)
			{
				pos += sprintf_s(buf + pos, sizeof(buf) - pos, "%-*s\n", LINE_WIDTH, "");
			}

			COORD origin = { 0, 0 };
			SetConsoleCursorPosition(hConsole, origin);
			DWORD written;
			WriteConsoleA(hConsole, buf, pos, &written, NULL);
		}

		//------------------------------------------------------------
		// 20초마다 하트비트 타이머 체크
		//------------------------------------------------------------
		/*if (tickCount >= 20)
		{
			tickCount = 0;

			uint64_t now64 = GetTickCount64();

			std::vector<SessionKey> expiredList;
			expiredList.reserve(128);

			InterlockedIncrement(&pServer->_activeInTimerLock);
			AcquireSRWLockShared(&pServer->_characterLock);
			for (auto& [key, ch] : pServer->_umapCharacter)
			{
				if (now64 - ch->_lastRecvTime >= 40000)
				{
					expiredList.push_back(ch->_sessionkey);
				}
			}
			ReleaseSRWLockShared(&pServer->_characterLock);
			InterlockedDecrement(&pServer->_activeInTimerLock);

			for (SessionKey& sk : expiredList)
			{
				LOG(L"ChattingServer", CSystemLog::LEVEL_ERROR,
					L"[Session:%llu] Heartbeat Timeout - Disconnect",
					sk.GetSessionId());
				InterlockedIncrement(&pServer->_heartbeatTimeoutCount);
				pServer->Disconnect(sk);
			}
		}*/
	}

	// 커서 복원
	cursorInfo.bVisible = TRUE;
	SetConsoleCursorInfo(hConsole, &cursorInfo);

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
