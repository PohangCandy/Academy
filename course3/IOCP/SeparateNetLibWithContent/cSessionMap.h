#pragma once
#include "stdafx.h"

//--------------------------------
//세션과 세션 ID를 저장하기 위한 맵 
// 싱글톤으로 만들어서, 세션 포인터 반환받는 작업에 락걸고 동기화
//--------------------------------
class SOCKETINFO;

class cSessionMap {
public:

	static cSessionMap* GetSessionMap();

	static void Destroy();

	void AddSession(SOCKETINFO* psession);

	void deleteSession(SOCKETINFO*& psession, char* s_ip, int i_port);

	void GetSessionptr(long long sessionId, SOCKETINFO*& sessionptr);

	long long GetSessionCount();

	void GetMapLock();

	void UnLockMap();

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
	long long _sessionCounter = 0;
	//CRITICAL_SECTION _sessionMap_cs;
};
