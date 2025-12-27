#pragma once
#include "stdafx.h"
#include <stack>
#include <set>
#include <unordered_map>
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
	DWORD AddSession(SOCKETINFO*& psession);
	//세션 맵 삭제 > 소켓 종료 > 세션 삭제
	void deleteSession(SOCKETINFO*& psession, char* s_ip, int i_port);
	//세션 맵 삭제 > 세션 삭제
	void OnlydeleteSession(SOCKETINFO*& psession, char* s_ip, int i_port);

	void GetSessionptr(DWORD sessionId, SOCKETINFO*& sessionptr);

	long long GetSize();

private:
	//싱글톤 인스턴스
	static cSessionMap* sessionMapInstance;

	cSessionMap();
	~cSessionMap() {};

	std::unordered_map<DWORD, SOCKETINFO*> m_sessionMap;

	DWORD _mapSize = 0;
};
