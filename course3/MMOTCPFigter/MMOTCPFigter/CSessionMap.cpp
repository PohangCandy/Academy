#include "cSessionMap.h"
#include "Session.h"
#include "MemoryPool.h"

cSessionMap* cSessionMap::sessionMapInstance = nullptr;
procademy::CMemoryPool<SOCKETINFO> SessionPool(20000, true);

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


long long cSessionMap::AddSession(SOCKETINFO*& psession)
{
	psession = SessionPool.Alloc();
	long long id;
	
	id = _mapSize++;
	m_sessionMap[id] = psession;
	
	return id;
}

void cSessionMap::deleteSession(SOCKETINFO*& psession, char* s_ip, int i_port)
{
	long long id = psession->session_id;
	
	if (m_sessionMap[id] == nullptr)
	{
		while (1)
		{
			printf("[deleteSession] duplicate id remove\n");
		}
	}

	m_sessionMap[id] = nullptr;
	m_sessionMap.erase(id);

	closesocket(psession->sock);
	SessionPool.Free(psession);
	psession = nullptr;
}

void cSessionMap::OnlydeleteSession(SOCKETINFO*& psession, char* s_ip, int i_port)
{
	long long id = psession->session_id;

	if (m_sessionMap[id] == nullptr)
	{
		while (1)
		{
			printf("[OnlydeleteSession] duplicate id remove\n");
		}
	}

	m_sessionMap[id] = nullptr;
	m_sessionMap.erase(id);
	//printf("[Network] 클라이언트 종료: IP 주소 = %s, 포트번호 = %d\n", s_ip, i_port);
	SessionPool.Free(psession);
	psession = nullptr;
}

void cSessionMap::GetSessionptr(long long sessionId, SOCKETINFO*& sessionptr)
{
	sessionptr = m_sessionMap[sessionId];
}

long long cSessionMap::GetSize()
{
	return m_sessionMap.size();
}

cSessionMap::cSessionMap()
{
	m_sessionMap.reserve(15000);
}

