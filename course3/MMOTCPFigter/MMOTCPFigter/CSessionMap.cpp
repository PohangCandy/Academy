#include "cSessionMap.h"
#include "Session.h"

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


long long cSessionMap::AddSession(SOCKETINFO* psession)
{
	long long id;
	EnterCriticalSection(&_sessionMap_cs);
	if (!_deletedIdStack.empty())
	{
		id = _deletedIdStack.top();
		_deletedIdStack.pop();
		LeaveCriticalSection(&_sessionMap_cs);

		if (_sessionMap[id] != nullptr)
		{
			while (1)
			{
				printf("[session Map] 삭제되지 않았는데 리스트에 할당됨. \n");
			}
		}
		_sessionMap[id] = psession;
	}
	else
	{
		LeaveCriticalSection(&_sessionMap_cs);
		id = _mapSize++;
		_sessionMap[id] = psession;
	}
	return id;
	//LeaveCriticalSection(&_sessionMap_cs);
}

void cSessionMap::deleteSession(SOCKETINFO*& psession, char* s_ip, int i_port)
{
	long long id = psession->session_id;
	

	//EnterCriticalSection(&_sessionMap_cs);
	_sessionMap[id] = nullptr;

	//누군가 세션 사용중인지 확인
	//psession->GetSessionLock();
	//psession->UnLockSession();
	closesocket(psession->sock);
	printf("[Network] 클라이언트 종료: IP 주소 = %s, 포트번호 = %d\n", s_ip, i_port);
	delete psession;
	psession = nullptr;

	EnterCriticalSection(&_sessionMap_cs);
	_deletedIdStack.push(id);

	LeaveCriticalSection(&_sessionMap_cs);
}

void cSessionMap::GetSessionptr(long long sessionId, SOCKETINFO*& sessionptr)
{
	//EnterCriticalSection(&_sessionMap_cs);
	sessionptr = _sessionMap[sessionId];
	if (sessionptr != nullptr)
	{
		sessionptr->GetSessionLock();
	}
	//LeaveCriticalSection(&_sessionMap_cs);
}

//long long cSessionMap::GetSessionCount()
//{
//	return _mapIndex;
//}

void cSessionMap::GetMapLock()
{
	//EnterCriticalSection(&_sessionMap_cs);
}

void cSessionMap::UnLockMap()
{
	//LeaveCriticalSection(&_sessionMap_cs);
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
