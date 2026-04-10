#pragma once
//------------------------------------------------------------
// CDBJobQueue - DB 워커 스레드용 MPSC/MPMC 작업 큐
//
// IOCP 워커 스레드는 동기 mysql_query 를 호출하면 묶이므로,
// 받은 LOGIN 요청을 이 큐에 enqueue 만 하고 빠져나간다.
// 별도의 DB 워커 스레드들이 큐에서 pop 하여 처리한다.
//
// 동기화: CRITICAL_SECTION + CONDITION_VARIABLE
//------------------------------------------------------------

#include "stdafx.h"
#include "SessionKey.h"
#include <queue>

enum eDBJobType
{
	eDBJob_Login = 1,
	// 추후: eDBJob_Logout, eDBJob_QueryUser ...
};

struct DBJob
{
	eDBJobType type;
	SessionKey sessionKey;          // 응답 보낼 대상 세션
	INT64      accountNo;
	char       sessionToken[64];    // 클라가 보낸 토큰 (현재 미검증)
};

class CDBJobQueue
{
public:
	CDBJobQueue();
	~CDBJobQueue();

	void Push(const DBJob& job);

	// true = job 채워짐, false = stop 신호로 깨어남
	bool Pop(DBJob& job);

	// 모든 대기 워커를 깨워서 종료시킴
	void Stop();

	int GetSize();

private:
	CRITICAL_SECTION   _cs;
	CONDITION_VARIABLE _cv;
	std::queue<DBJob>  _q;
	volatile bool      _bStopped;
};
