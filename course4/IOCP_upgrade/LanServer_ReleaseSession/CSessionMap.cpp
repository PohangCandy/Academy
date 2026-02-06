#include "cSessionMap.h"
#include "Session.h"
#include "MemoryPoolForLockFree.h"
//#include "CLockFreeStack.h"

#define SESSION_ID_BIT (44) 
#define SESSION_INDEX_BIT  (20)//대충 한 백만?

#define SESSION_ID_BITMASK ((1ULL << SESSION_ID_BIT) - 1)
#define SESSION_INDEX_BITMASK  (((1ULL << SESSION_INDEX_BIT) - 1) << SESSION_ID_BITMASK)

#define RELEASE_FLAGBIT 31
#define RELEASE_FLAG      (1u << RELEASE_FLAGBIT)  
#define RELEASE_FLAG_MASK (RELEASE_FLAG - 1)    

	//--------------------------------
	// 세션과 세션 ID를 저장하기 위한 맵 
	// 자료구조 : 배열
	// 최대치 : 8byte 크기
	//--------------------------------
	//SOCKETINFO* _sessionMap[1 << 24] = {};
	//세션의 연결이 끊겼다고 해서 바로 세션을 종료시키는게 아님.
	// IO카운팅이 끊나야 세션을 종료시키므로 대충 한 10만명 받을 수 있도록 만들어둬야
	// 유니크한 세션 ID와 인덱스를 조합시켜야 하므로
	// 세션 ID는 long long -> 8바이트 = 약 64비트
	// 인덱스로 적당히 한 20비트 사용, sessionid 44비트는 
SOCKETINFO _sessionMap[100000] = {};

//CLockFreeStack _deletedIdStack;

cSessionMap* cSessionMap::sessionMapInstance = nullptr;

cSessionMap* cSessionMap::GetSessionMap()
{
	if (sessionMapInstance == nullptr)
	{
		sessionMapInstance = new cSessionMap;
		atexit(Destroy);
	}
	return sessionMapInstance;
}

void cSessionMap::Destroy()
{
	delete sessionMapInstance;
	sessionMapInstance = nullptr;
}


SOCKETINFO* cSessionMap::AllocSessionptr(SOCKET sock)
{
	//AcceptThread가 여러개있다면 맵에 추가하는 과정도 락/락프리를 통해 이뤄져야 한다.

	long long session_id;
	long long id_Bit;
	long long index_Bit;

	EnterCriticalSection(&_sessionMap_cs);
	if (!_deletedIdStack.empty())
	{
		session_id = _deletedIdStack.top();
		_deletedIdStack.pop();

		index_Bit = GetSessionIndex(session_id);

		if (_sessionMap[index_Bit]._Active == true)
		{
			printf("[session Map] 삭제되지 않았는데 리스트에 할당됨. \n");
			__debugbreak();
		}

		
	}
	else
	{
		index_Bit = _InterlockedIncrement64(&_nextIndex);
	}
	id_Bit = _InterlockedIncrement64(&_nextSessionID);
	
	session_id = MakeSessionKey(index_Bit, id_Bit);
	_sessionMap[index_Bit].Inintialize(sock, session_id);
	LeaveCriticalSection(&_sessionMap_cs);

	return &_sessionMap[index_Bit];
}

void cSessionMap::FreeSession(SOCKETINFO* psession, char* s_ip, int i_port)
{
	long long session_id;
	long long id_Bit;
	long long index_Bit;

	session_id = psession->session_id;
	index_Bit = GetSessionIndex(session_id);

	_sessionMap[index_Bit]._Active = false;

	closesocket(psession->_sock);
	//printf("[Network] 클라이언트 종료: IP 주소 = %s, 포트번호 = %d\n", s_ip, i_port);

	EnterCriticalSection(&_sessionMap_cs);
	_deletedIdStack.push(index_Bit);

	id_Bit = GetSessionId(session_id);
	printf("[Network] 클라이언트 종료: ID  = %d\n", id_Bit);

	LeaveCriticalSection(&_sessionMap_cs);
}

SOCKETINFO* cSessionMap::GetSessionptr(long long key)
{
	long long index = GetSessionIndex(key);
	long long sessionId = GetSessionId(key);

	SOCKETINFO* ptr = &_sessionMap[index];

	// 1. 세션 ID 검증
	if (ptr->session_id != sessionId)
		return nullptr;

	// 2. IOCount 증가 시도 (ReleaseFlag 체크)
	if (!IncreaseSessionIO(ptr))
		return nullptr;

	// 3. 다시 한 번 세션 ID 검증 (ABA 방지)
	if (ptr->session_id != sessionId)
	{
		DecreaseSessionIO(ptr);
		return nullptr;
	}

	return ptr;
}

void cSessionMap::ReleaseSession(SOCKADDR_IN& clientaddr, SOCKETINFO* ptr)
{
	FreeSession(ptr, inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
}

bool cSessionMap::DecreaseSessionIO(SOCKADDR_IN& clientaddr, SOCKETINFO* ptr)
{
	unsigned long oldVal;
	unsigned long newVal;

	while (true)
	{
		oldVal = ptr->IOCount;

		// 이미 Release 상태면 아무것도 하지 않음
		if (oldVal & RELEASE_FLAG)
			return false;

		unsigned long io = oldVal & RELEASE_FLAG_MASK;
		if (io == 0)
		{
			__debugbreak(); // underflow
			return false;
		}

		newVal = oldVal - 1;

		// IOCount 감소 성공?
		if (InterlockedCompareExchange(
			(unsigned long*)&ptr->IOCount,
			newVal,
			oldVal) == oldVal)
		{
			break;
		}
	}

	// 감소 후 IOCount == 0 이고 ReleaseFlag == 0 이면
	if ((newVal & RELEASE_FLAG_MASK) == 0)
	{
		// ReleaseFlag 세팅 시도
		if (InterlockedCompareExchange(
			(unsigned long*)&ptr->IOCount,
			newVal | RELEASE_FLAG,
			newVal) == newVal)
		{
			ReleaseSession(clientaddr, ptr);
			return false;
		}
	}

	return true;
}

bool cSessionMap::IncreaseSessionIO(SOCKETINFO* ptr)
{
	unsigned long oldVal;
	unsigned long newVal;

	while (true)
	{
		oldVal = ptr->IOCount;

		// 이미 Release 상태면 IO 추가 불가
		if (oldVal & RELEASE_FLAG)
			return false;

		unsigned long io = oldVal & RELEASE_FLAG_MASK;
		if (io == RELEASE_FLAG_MASK)
		{
			__debugbreak(); // overflow
			return false;
		}

		newVal = oldVal + 1;

		if (InterlockedCompareExchange(
			(unsigned long*)&ptr->IOCount,
			newVal,
			oldVal) == oldVal)
		{
			return true;
		}
	}
}

long long cSessionMap::GetnextSessionKey()
{
	return _nextSessionID;
}

cSessionMap::cSessionMap()
{
	InitializeCriticalSection(&_sessionMap_cs);
}

cSessionMap::~cSessionMap()
{
	DeleteCriticalSection(&_sessionMap_cs);
}

long long cSessionMap::GetSessionId(long long key)
{
	return key & SESSION_ID_BITMASK;
}

long long cSessionMap::GetSessionIndex(long long key)
{
	return (key & SESSION_INDEX_BITMASK) >> SESSION_ID_BIT;
}

long long cSessionMap::MakeSessionKey(long long index, long long sessionId)
{
	return (index << SESSION_ID_BIT) | (sessionId & SESSION_ID_BITMASK);
}

//void cSessionMap::GetMapLock()
//{
//	EnterCriticalSection(&_sessionMap_cs);
//}
//
//void cSessionMap::UnLockMap()
//{
//	LeaveCriticalSection(&_sessionMap_cs);
//}