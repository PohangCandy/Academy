#pragma once
//------------------------------------------------------------
// CDBJobQueue - DB 워커 스레드용 작업 큐
//
// IOCP 워커 스레드는 동기 mysql_query 를 호출하면 묶이므로,
// 받은 LOGIN 요청을 이 큐에 enqueue 만 하고 빠져나간다.
// 별도의 DB 워커 스레드들이 큐에서 pop 하여 처리한다.
//
// 동기화: std::mutex + std::condition_variable (C++11)
//   -> Win32 CONDITION_VARIABLE 보다 이식성/빌드 안정성 우수
//------------------------------------------------------------

#include "stdafx.h"
#include <queue>
#include <mutex>
#include <condition_variable>
#include <cstdint>
#include "SessionKey.h"

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

	bool Pop(DBJob& job);

	void Stop();

	int  GetSize();

private:
	std::mutex              _mtx;
	std::condition_variable _cv;
	std::queue<DBJob>       _q;
	bool                    _bStopped;
};
