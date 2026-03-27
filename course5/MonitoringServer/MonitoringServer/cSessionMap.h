#pragma once
#include "stdafx.h"
#include <stack>

//------------------------------------------------------------
// [변경사항] 싱글턴 패턴 제거 → 인스턴스 기반
// 이유: 모니터링 서버에서 LAN서버와 NET서버가 각각 독립적인
//       세션맵을 가져야 하므로 싱글턴으로는 불가능.
//       생성자에 capacity를 전달하여 서버별 적합한 크기로 할당.
//------------------------------------------------------------

class SOCKETINFO;
struct SessionKey;
enum class ReleaseResult : uint8_t;

class cSessionMap {
public:

	cSessionMap(int capacity);
	~cSessionMap();

	SOCKETINFO* AllocSessionptr(SOCKET sock);
	SOCKETINFO* GetSessionptr(SessionKey key);
	void FreeSession(SOCKETINFO* psession);

	ReleaseResult DecreaseSessionIO(SOCKETINFO* ptr);
	bool IncreaseSessionIO(SOCKETINFO* ptr);

	long long GetnextSessionKey();

private:

	SOCKETINFO* _sessionArray;
	int _capacity;

	long long _nextSessionID = 0;
	long long _nextIndex = 0;

	std::stack<uint32_t> _deletedSessionIndex;
	CRITICAL_SECTION _sessionMap_cs;
};
