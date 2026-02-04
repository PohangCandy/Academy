#include "cSessionMap.h"
#include "Session.h"
#include "MemoryPoolForLockFree.h"
#include "CLockFreeStack.h"

#define BUFSIZE (1024 * 1024)

CLockFreeStack _deletedIdStack;

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

SOCKETINFO* cSessionMap::MakeNewSession(SOCKET sock)
{
	SOCKETINFO* nSession = new SOCKETINFO(BUFSIZE);
	long long id = InsertSessionptrToSessionMap(nSession);
	nSession->Inintialize(sock, id);

	return nSession;
}


long long cSessionMap::InsertSessionptrToSessionMap(SOCKETINFO* psession)
{
	//AcceptThread가 여러개있다면 맵에 추가하는 과정도 락/락프리를 통해 이뤄져야 한다.
	long long id;

	//EnterCriticalSection(&_sessionMap_cs);
	if (!_deletedIdStack.empty())
	{
		int* top = _deletedIdStack.pop();
		id = (long long) *top;
		//_deletedIdStack.pop();
		//LeaveCriticalSection(&_sessionMap_cs);

		if (_sessionMap[id] != nullptr)
		{
			printf("[session Map] 삭제되지 않았는데 리스트에 할당됨. \n");
			__debugbreak();
		}

		_sessionMap[id] = psession;
	}
	else
	{
		id = _InterlockedIncrement64(&_mapSize);
		_sessionMap[id] = psession;
		//LeaveCriticalSection(&_sessionMap_cs);
	}
	return id;
}

void cSessionMap::deleteSessionptrFromSessionMap(SOCKETINFO*& psession, char* s_ip, int i_port)
{
	long long id = psession->session_id;
	
	_sessionMap[id] = nullptr;

	closesocket(psession->_sock);
	//printf("[Network] 클라이언트 종료: IP 주소 = %s, 포트번호 = %d\n", s_ip, i_port);
	
	delete psession;
	psession = nullptr;

	//EnterCriticalSection(&_sessionMap_cs);
	if (_sessionMap[id] != nullptr)
	{
		__debugbreak();
	}
	_deletedIdStack.push(id);
	printf("[Network] 클라이언트 종료: ID  = %d\n", id);

	//LeaveCriticalSection(&_sessionMap_cs);
}

void cSessionMap::GetSessionptr(long long sessionId, SOCKETINFO*& sessionptr)
{
	sessionptr = _sessionMap[sessionId];
	//if (sessionptr != nullptr)
	//{
	//	sessionptr->GetSessionLock();
	//}
}

long long cSessionMap::GetSize()
{
	return _mapSize;
}

cSessionMap::cSessionMap()
{
	InitializeCriticalSection(&_sessionMap_cs);
}

cSessionMap::~cSessionMap()
{
	DeleteCriticalSection(&_sessionMap_cs);
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