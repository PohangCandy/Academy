#include "cSessionMap.h"
#include "Session.h"
#include "MemoryPool.h"

cSessionMap* cSessionMap::sessionMapInstance = nullptr;
procademy::CMemoryPool<SOCKETINFO> SessionPool(15000, true);

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


DWORD cSessionMap::AddSession(SOCKETINFO*& psession)
{
	psession = SessionPool.Alloc();
	psession->OnAccept();
	DWORD id;
	
	id = _mapSize++;
	m_sessionMap[id] = psession;
	
	return id;
}

void cSessionMap::deleteSession(SOCKETINFO*& psession, char* s_ip, int i_port)
{
	long long id = psession->session_id;
	
	// 1. find를 사용하여 존재 여부 확인
	auto it = m_sessionMap.find(id);

	if (it == m_sessionMap.end()) // 키가 존재하지 않는 경우
	{
		// 에러 처리 또는 로그 출력
		printf("[deleteSession] Session ID %lld not found\n", id);
		return;
	}

	//m_sessionMap[id] = nullptr;
	m_sessionMap.erase(it);

	closesocket(psession->sock);
	psession->OnRelease();
	SessionPool.Free(psession);
	psession = nullptr;
}

void cSessionMap::GetSessionptr(DWORD sessionId, SOCKETINFO*& sessionptr)
{
	auto it = m_sessionMap.find(sessionId);
	if (it == m_sessionMap.end())
	{
		sessionptr = nullptr;
		return;
	}
	sessionptr = it->second;
	return;
}

long long cSessionMap::GetSize()
{
	return m_sessionMap.size();
}

cSessionMap::cSessionMap()
{
	m_sessionMap.reserve(15000);
}

