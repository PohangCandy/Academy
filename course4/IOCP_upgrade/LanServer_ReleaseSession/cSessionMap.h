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

	//return newSessionptr
	SOCKETINFO* MakeNewSession(SOCKET sock);

	//return SessionId
	long long InsertSessionptrToSessionMap(SOCKETINFO* psession);

	void deleteSessionptrFromSessionMap(SOCKETINFO*& psession, char* s_ip, int i_port);

	void GetSessionptr(long long sessionId, SOCKETINFO*& sessionptr);

	//long long GetSessionCount();

	//void GetMapLock();

	//void UnLockMap();

	long long GetSize();

private:

	//싱글톤 인스턴스
	static cSessionMap* sessionMapInstance;

	cSessionMap();
	~cSessionMap();

	//--------------------------------
	// 세션과 세션 ID를 저장하기 위한 맵 
	// 자료구조 : 배열
	// 최대치 : 8byte 크기
	//--------------------------------
	SOCKETINFO* _sessionMap[1 << 24] = {};
	//세션의 연결이 끊겼다고 해서 바로 세션을 종료시키는게 아님.
	// IO카운팅이 끊나야 세션을 종료시키므로 대충 한 10만명 받을 수 있도록 만들어둬야
	// 유니크한 세션 ID와 인덱스를 조합시켜야 하므로
	// 세션 ID는 long long -> 8바이트 = 약 64비트
	// 인덱스로 적당히 한 20비트 사용, sessionid 44비트는 
	//SOCKETINFO _sessionMap[100000] = {};
	
	//long long _mapIndex = 0;
	long long _mapSize = 0;
	std::stack<long long> _deletedIdStack;
	CRITICAL_SECTION _sessionMap_cs;
};
