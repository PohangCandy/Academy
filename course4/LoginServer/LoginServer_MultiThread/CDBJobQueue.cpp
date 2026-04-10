#include "CDBJobQueue.h"

CDBJobQueue::CDBJobQueue()
	: _bStopped(false)
{
}

CDBJobQueue::~CDBJobQueue()
{
}

void CDBJobQueue::Push(const DBJob& job)
{
	{
		std::lock_guard<std::mutex> lk(_mtx);
		_q.push(job);
	}
	// 한 명만 깨우면 충분 (FIFO 처리)
	_cv.notify_one();
}

bool CDBJobQueue::Pop(DBJob& job)
{
	std::unique_lock<std::mutex> lk(_mtx);
	_cv.wait(lk, [this]() { return !_q.empty() || _bStopped; });

	if (_bStopped && _q.empty())
		return false;

	job = _q.front();
	_q.pop();
	return true;
}

void CDBJobQueue::Stop()
{
	{
		std::lock_guard<std::mutex> lk(_mtx);
		_bStopped = true;
	}
	// 대기 중인 모든 워커를 깨움
	_cv.notify_all();
}

int CDBJobQueue::GetSize()
{
	std::lock_guard<std::mutex> lk(_mtx);
	return (int)_q.size();
}
