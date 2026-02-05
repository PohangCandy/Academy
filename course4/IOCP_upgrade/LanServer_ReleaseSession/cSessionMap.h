#pragma once
#include "stdafx.h"
#include <stack>
//--------------------------------
//세션과 세션 ID를 저장하기 위한 맵 
// 싱글톤으로 만들어서, 세션 포인터 반환받는 작업에 락걸고 동기화
//--------------------------------
class SOCKETINFO;

class cSessionMap {
public:

	static cSessionMap* GetSessionMap();

	static void Destroy();

	//return SessionId
	SOCKETINFO* AllocSessionptr(SOCKET sock);

	void FreeSession(SOCKETINFO* psession, char* s_ip, int i_port);

	SOCKETINFO* GetSessionptr(long long sessionId);

	//long long GetSessionCount();

	//void GetMapLock();

	//void UnLockMap();

	long long GetnextSessionKey();

private:

	//싱글톤 인스턴스
	static cSessionMap* sessionMapInstance;

	cSessionMap();
	~cSessionMap();

	long long GetSessionId(long long  key);

	long long GetSessionIndex(long long  key);

	long long MakeSessionKey(long long  index, long long sessionId);

	
	//long long _mapIndex = 0;
	long long _nextSessionID = 0;
	long long _nextIndex = 0;

	std::stack<long long> _deletedIdStack;
	CRITICAL_SECTION _sessionMap_cs;
};
