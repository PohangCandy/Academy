#pragma once
#include "stdafx.h"
#include <stack>
//#include "SessionKey.h"
//--------------------------------
//세션과 세션 ID를 저장하기 위한 맵 
// 싱글톤으로 만들어서, 세션 포인터 반환받는 작업에 락걸고 동기화
//--------------------------------
class SOCKETINFO;
struct SessionKey;

class cSessionMap {
public:

	static cSessionMap* GetSessionMap();

	static void Destroy();

	//return SessionId
	SOCKETINFO* AllocSessionptr(SOCKET sock);

	void FreeSession(SOCKETINFO* psession);

	SOCKETINFO* GetSessionptr(SessionKey key);

	//long long GetSessionCount();

	//void GetMapLock();

	//void UnLockMap();

		//-----------------------------------------
	// 세션 종료
	// IO가 끝난 세션에 대해 완전히 삭제
	//-----------------------------------------
	void ReleaseSession(SOCKETINFO* ptr);

	//-----------------------------------------
	// 세션 IOCount를 줄이는 함수
	// -> interlock해도 되지만 곳곳에 ReleaseSession이 뿌려져 있는게 마음에 들지 않아서 묶음.
	// 원래는 무조건 ReleaseSession을 진행시킨다였지만 이젠 경우에 따라 ReleaseSession이 진행 되니 않고 그냥 decrease만 하는 경우도 존재
	// decreaseIO를 한 결과가 false면 ReleaseSession 성공으로 간주한다.
	//-----------------------------------------
	bool DecreaseSessionIO(SOCKETINFO* ptr);

	//-----------------------------------------
	// 세션 IOCount를 증가시키는 함수
	// ReleaseFlag 비트를 제외한 나머지 비트에 대해서만 증가를 시킨다.
	//-----------------------------------------
	bool IncreaseSessionIO(SOCKETINFO* ptr);


	long long GetnextSessionKey();

private:

	//싱글톤 인스턴스
	static cSessionMap* sessionMapInstance;

	cSessionMap();
	~cSessionMap();
	
	//long long _mapIndex = 0;
	long long _nextSessionID = 0;
	long long _nextIndex = 0;

	std::stack<uint32_t> _deletedSessionIndex;
	CRITICAL_SECTION _sessionMap_cs;
};
