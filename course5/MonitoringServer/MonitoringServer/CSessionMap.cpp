#include "cSessionMap.h"
#include "SessionKey.h"
#include "Session.h"
#include "CPacketRingBuffer.h"
#include "CPacketForMultiThread.h"
#include "CommonProtocol.h"

#define RELEASE_FLAGBIT 31
#define RELEASE_FLAG      (1u << RELEASE_FLAGBIT)
#define RELEASE_FLAG_MASK (RELEASE_FLAG - 1)

//------------------------------------------------------------
// [변경사항] 싱글턴 → 인스턴스 기반
// 세션 배열을 전역이 아닌 heap에 동적 할당
// capacity를 생성자 인자로 받아 서버별 적합한 크기 사용
//------------------------------------------------------------

cSessionMap::cSessionMap(int capacity)
	: _capacity(capacity), _nextSessionID(0), _nextIndex(0)
{
	InitializeCriticalSection(&_sessionMap_cs);
	_sessionArray = new SOCKETINFO[capacity];
}

cSessionMap::~cSessionMap()
{
	delete[] _sessionArray;
	_sessionArray = nullptr;
	DeleteCriticalSection(&_sessionMap_cs);
}

SOCKETINFO* cSessionMap::AllocSessionptr(SOCKET sock)
{
	SessionKey session_key;
	uint32_t id_Bit;
	long long index_Bit;

	EnterCriticalSection(&_sessionMap_cs);
	if (!_deletedSessionIndex.empty())
	{
		index_Bit = _deletedSessionIndex.top();
		_deletedSessionIndex.pop();

		if (_sessionArray[index_Bit]._Active == true)
		{
			printf("[SessionMap] Active session in deleted stack.\n");
			__debugbreak();
		}
	}
	else
	{
		index_Bit = _nextIndex++;

		if (index_Bit >= _capacity)
		{
			printf("[SessionMap] Session capacity exceeded! (capacity=%d)\n", _capacity);
			LeaveCriticalSection(&_sessionMap_cs);
			__debugbreak();
			return nullptr;
		}
	}
	id_Bit = (uint32_t)(++_nextSessionID);

	session_key = SessionKey::MakeKey((uint32_t)index_Bit, id_Bit);
	_sessionArray[index_Bit].Inintialize(sock, session_key);
	LeaveCriticalSection(&_sessionMap_cs);

	return &_sessionArray[index_Bit];
}

void cSessionMap::FreeSession(SOCKETINFO* psession)
{
	SessionKey session_key;
	uint32_t index_Bit;

	session_key = psession->_sessionKey;
	index_Bit = session_key.GetIndex();

	_sessionArray[index_Bit]._Active = false;

	// SendBuf 잔여 패킷 정리
	psession->_sendBuf->Lock();
	int remain = psession->_sendBuf->GetUseSize();
	int front = psession->_sendBuf->GetFront();
	int cap = psession->_sendBuf->GetBufferSize();
	CPacket** buf = psession->_sendBuf->GetBufPtr();
	for (int i = 0; i < remain; i++)
	{
		buf[front]->SubRef();
		front = (front + 1) % cap;
	}
	psession->_sendBuf->MoveFront(remain);
	psession->_sendBuf->UnLock();

	closesocket(psession->_sock);

	EnterCriticalSection(&_sessionMap_cs);
	_deletedSessionIndex.push(index_Bit);
	LeaveCriticalSection(&_sessionMap_cs);
}

SOCKETINFO* cSessionMap::GetSessionptr(SessionKey key)
{
	uint32_t index = key.GetIndex();
	uint64_t sessionId = key.GetSessionId();

	if (index >= (uint32_t)_capacity)
		return nullptr;

	SOCKETINFO* ptr = &_sessionArray[index];

	if (!IncreaseSessionIO(ptr))
		return nullptr;

	if (ptr->_sessionKey.GetSessionId() != sessionId)
	{
		// 재활용된 세션 — IOCount 증가분만 되돌림, Release 로직 금지
		InterlockedDecrement((unsigned long*)&ptr->_IOCount);
		return nullptr;
	}

	return ptr;
}


ReleaseResult cSessionMap::DecreaseSessionIO(SOCKETINFO* ptr)
{
	unsigned long oldVal;
	unsigned long newVal;

	while (true)
	{
		oldVal = ptr->_IOCount;

		if (oldVal & RELEASE_FLAG)
			return ReleaseResult::Fail;

		unsigned long io = oldVal & RELEASE_FLAG_MASK;
		if (io == 0)
		{
			__debugbreak();
			return ReleaseResult::Fail;
		}

		newVal = oldVal - 1;

		if (InterlockedCompareExchange(
			(unsigned long*)&ptr->_IOCount,
			newVal,
			oldVal) == oldVal)
		{
			break;
		}
	}

	if ((newVal & RELEASE_FLAG_MASK) == 0)
	{
		if (InterlockedCompareExchange(
			(unsigned long*)&ptr->_IOCount,
			newVal | RELEASE_FLAG,
			newVal) == newVal)
		{
			FreeSession(ptr);
			return ReleaseResult::Released;
		}
	}

	return ReleaseResult::Success;
}

bool cSessionMap::IncreaseSessionIO(SOCKETINFO* ptr)
{
	unsigned long oldVal;
	unsigned long newVal;

	while (true)
	{
		oldVal = ptr->_IOCount;

		if (oldVal & RELEASE_FLAG)
			return false;

		unsigned long io = oldVal & RELEASE_FLAG_MASK;
		if (io == RELEASE_FLAG_MASK)
		{
			__debugbreak();
			return false;
		}

		newVal = oldVal + 1;

		if (InterlockedCompareExchange(
			(unsigned long*)&ptr->_IOCount,
			newVal,
			oldVal) == oldVal)
		{
			return true;
		}
	}
}

long long cSessionMap::GetnextSessionKey()
{
	return _nextSessionID;
}
