#include "CDBJobQueue.h"

CDBJobQueue::CDBJobQueue()
	: _bStopped(false)
{
	InitializeCriticalSection(&_cs);
	InitializeConditionVariable(&_cv);
}

CDBJobQueue::~CDBJobQueue()
{
	DeleteCriticalSection(&_cs);
}

void CDBJobQueue::Push(const DBJob& job)
{
	EnterCriticalSection(&_cs);
	_q.push(job);
	LeaveCriticalSection(&_cs);

	// 한 명만 깨우면 충분 (FIFO 처리)
	WakeConditionVariable(&_cv);
}

bool CDBJobQueue::Pop(DBJob& job)
{
	EnterCriticalSection(&_cs);
	while (_q.empty() && !_bStopped)
	{
		SleepConditionVariableCS(&_cv, &_cs, INFINITE);
	}

	if (_bStopped && _q.empty())
	{
		LeaveCriticalSection(&_cs);
		return false;
	}

	job = _q.front();
	_q.pop();
	LeaveCriticalSection(&_cs);
	return true;
}

void CDBJobQueue::Stop()
{
	EnterCriticalSection(&_cs);
	_bStopped = true;
	LeaveCriticalSection(&_cs);

	// 대기 중인 모든 워커를 깨움
	WakeAllConditionVariable(&_cv);
}

int CDBJobQueue::GetSize()
{
	EnterCriticalSection(&_cs);
	int s = (int)_q.size();
	LeaveCriticalSection(&_cs);
	return s;
}
