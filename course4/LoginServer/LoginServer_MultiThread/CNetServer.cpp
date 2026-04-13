#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma comment(lib, "ws2_32")

// WSASend 호출 1회에 묶을 수 있는 WSABUF 개수 상한.
// CLanClient.cpp 와 동일한 값을 사용한다.
#define dfSEND_WSABUF_MAX	(128)

#include "CNetServer.h"
#include "CommonProtocol.h"
#include "cSessionMap.h"
#include "Session.h"
#include "CRingBuffer.h"
#include "CPacketRingBuffer.h"
#include "OVERLAPPED_CONTEXT.h"
#include "errlog.h"
#include "CPacketForMultiThread.h"
#include "CSystemLog.h"
#include <ws2tcpip.h>

CNetServer::CNetServer()
	: _hWorkerThreadIOCP(NULL), _listenSock(INVALID_SOCKET), _pSessionMap(nullptr)
{
}

CNetServer::~CNetServer()
{
	Stop();
}

bool CNetServer::Start(int port, int maxSession, int workerThreadCount)
{
	int retval;

	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return false;

	_maxSession = maxSession;
	_pSessionMap = new cSessionMap(maxSession);

	SYSTEM_INFO si;
	GetSystemInfo(&si);

	// 물리 코어 수 (논리 프로세서 / 2)
	int numCores = (int)si.dwNumberOfProcessors / 2;
	if (numCores < 1) numCores = 1;
	int concurrency = numCores;      // IOCP 동시 실행 스레드 제한

	// 워커 스레드 수: 인자 > 0 이면 그 값, 아니면 자동
	if (workerThreadCount > 0)
	{
		_workerThreadCount = workerThreadCount;
		if (_workerThreadCount > MAX_WORKER_THREADS)
			_workerThreadCount = MAX_WORKER_THREADS;
	}
	else
	{
		_workerThreadCount = numCores * 2;  // I/O 대기 여유분 포함
	}

	_hWorkerThreadIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, concurrency);
	if (_hWorkerThreadIOCP == NULL) return false;

	_isRunning = true;

	unsigned int uiThreadID;
	memset(_hWorkerThreads, 0, sizeof(_hWorkerThreads));

	for (int i = 0; i < _workerThreadCount; i++)
	{
		_hWorkerThreads[i] = (HANDLE)_beginthreadex(NULL, 0, WorkerThread, this, 0, &uiThreadID);
		if (_hWorkerThreads[i] == NULL) return false;
	}

	// 모니터 스레드
	_hMonitorThread = (HANDLE)_beginthreadex(NULL, 0, MonitorThread, this, 0, &uiThreadID);
	if (_hMonitorThread == NULL) return false;

	// Listen 소켓
	_listenSock = socket(AF_INET, SOCK_STREAM, 0);
	if (_listenSock == INVALID_SOCKET) { err_quit("NetServer socket()"); return false; }

	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(port);
	retval = bind(_listenSock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) { err_quit("NetServer bind()"); return false; }

	retval = listen(_listenSock, SOMAXCONN);
	if (retval == SOCKET_ERROR) { err_quit("NetServer listen()"); return false; }

	// Listening started

	// Accept 스레드
	_hAcceptThread = (HANDLE)_beginthreadex(NULL, 0, AcceptThread, this, 0, &uiThreadID);
	if (_hAcceptThread == NULL) return false;

	return true;
}

void CNetServer::Stop()
{
	// 1. 리슨소켓 닫기 -> Accept 스레드 종료, 신규 접속 차단
	if (_listenSock != INVALID_SOCKET)
	{
		closesocket(_listenSock);
		_listenSock = INVALID_SOCKET;
	}

	// Accept 스레드 종료 대기
	if (_hAcceptThread != NULL)
	{
		WaitForSingleObject(_hAcceptThread, INFINITE);
		CloseHandle(_hAcceptThread);
		_hAcceptThread = NULL;
	}

	// 2. 모든 기존 세션 Disconnect
	if (_pSessionMap != nullptr)
	{
		for (int i = 0; i < _maxSession; i++)
		{
			SOCKETINFO* ptr = _pSessionMap->GetSessionptrByIndex(i);
			if (ptr != nullptr && ptr->_Active)
			{
				Disconnect(ptr->_sessionKey);
			}
		}

		// 세션이 모두 정리될 때까지 대기
		int waitCount = 0;
		while (_sessionCount.load() > 0 && waitCount < 50)
		{
			Sleep(100);
			waitCount++;
		}
	}

	// 3. 워커 스레드 종료 (PQCS로 종료 신호) + 실제 종료 대기
	for (int i = 0; i < _workerThreadCount; i++)
	{
		PostQueuedCompletionStatus(_hWorkerThreadIOCP, 0, 0, NULL);
	}
	WaitForMultipleObjects(_workerThreadCount, _hWorkerThreads, TRUE, INFINITE);
	for (int i = 0; i < _workerThreadCount; i++)
	{
		CloseHandle(_hWorkerThreads[i]);
		_hWorkerThreads[i] = NULL;
	}

	// 4. 모니터 스레드 종료 대기
	_isRunning = false;
	if (_hMonitorThread != NULL)
	{
		WaitForSingleObject(_hMonitorThread, INFINITE);
		CloseHandle(_hMonitorThread);
		_hMonitorThread = NULL;
	}

	// 5. 리소스 정리
	if (_hWorkerThreadIOCP != NULL)
	{
		CloseHandle(_hWorkerThreadIOCP);
		_hWorkerThreadIOCP = NULL;
	}
	if (_pSessionMap != nullptr)
	{
		delete _pSessionMap;
		_pSessionMap = nullptr;
	}
}

int CNetServer::GetSessionCount()
{
	return _sessionCount;
}

unsigned int __stdcall CNetServer::AcceptThread(LPVOID arg)
{
	CNetServer* pServer = (CNetServer*)arg;

	SOCKET client_sock;
	SOCKADDR_IN clientaddr;
	int addrlen;
	int retval;
	DWORD recvbytes, flags;

	while (1)
	{
		addrlen = sizeof(clientaddr);
		client_sock = accept(pServer->_listenSock, (SOCKADDR*)&clientaddr, &addrlen);
		if (client_sock == INVALID_SOCKET)
		{
			break;
		}

		pServer->_acceptCount.fetch_add(1, std::memory_order_relaxed);
		pServer->_totalAcceptCount.fetch_add(1, std::memory_order_relaxed);

		LINGER optval;
		optval.l_onoff = 1;
		optval.l_linger = 0;
		retval = setsockopt(client_sock, SOL_SOCKET, SO_LINGER, (char*)&optval, sizeof(optval));
		if (retval == SOCKET_ERROR)
		{
			closesocket(client_sock);
			continue;
		}

		char ipStr[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &clientaddr.sin_addr, ipStr, sizeof(ipStr));

		if (!pServer->OnConnectionRequest(ipStr, ntohs(clientaddr.sin_port)))
		{
			closesocket(client_sock);
			continue;
		}

		SOCKETINFO* ptr = pServer->_pSessionMap->AllocSessionptr(client_sock);
		if (ptr == nullptr)
		{
			closesocket(client_sock);
			continue;
		}

		ptr->_IP = ipStr;
		ptr->_PORT = ntohs(clientaddr.sin_port);

		pServer->_sessionCount.fetch_add(1, std::memory_order_relaxed);

		pServer->OnClientJoin(clientaddr, ptr->_sessionKey);

		CreateIoCompletionPort((HANDLE)client_sock, pServer->_hWorkerThreadIOCP, (ULONG_PTR)ptr, 0);

		//------------------------------------------------------------
		// _IOCount는 AllocSessionptr에서 이미 1로 초기화됨 (AcceptThread 소유권)
		// WSARecv를 위해 +1 -> IOCount = 2
		//------------------------------------------------------------
		if (!pServer->_pSessionMap->IncreaseSessionIO(ptr))
		{
			// 이미 Release된 상태 - 초기 IOCount=1인데 여기서 실패하면 논리 오류
			__debugbreak();
		}

		WSABUF wsabuf;
		wsabuf.buf = ptr->_recvBuf->GetRearBufferPtr();
		wsabuf.len = ptr->_recvBuf->DirectEnqueueSize();
		flags = 0;
		retval = WSARecv(client_sock, &wsabuf, 1, &recvbytes, &flags, (LPWSAOVERLAPPED)ptr->_recvOverlapped, NULL);
		if (retval == SOCKET_ERROR)
		{
			if (WSAGetLastError() != ERROR_IO_PENDING)
			{
				LOG(L"NetServer", CSystemLog::LEVEL_ERROR,
					L"[Session:%llu] AcceptThread initial WSARecv failed (err:%d)",
					ptr->_sessionKey.GetSessionId(), WSAGetLastError());
				// WSARecv 실패 - WSARecv의 IOCount(+1분) 회수
				SessionKey origin = ptr->_sessionKey;
				if (pServer->_pSessionMap->DecreaseSessionIO(ptr) == ReleaseResult::Released)
				{
					pServer->_sessionCount.fetch_sub(1, std::memory_order_relaxed);
					pServer->OnClientLeave(origin);
				}
			}
		}

		//------------------------------------------------------------
		// AcceptThread 소유권 반환 (초기 IOCount=1 분)
		// 이 시점에서 모든 초기화 완료 - 세션 해제 허용
		//------------------------------------------------------------
		{
			SessionKey origin = ptr->_sessionKey;
			if (pServer->_pSessionMap->DecreaseSessionIO(ptr) == ReleaseResult::Released)
			{
				pServer->_sessionCount.fetch_sub(1, std::memory_order_relaxed);
				pServer->OnClientLeave(origin);
			}
		}
	}

	// AcceptThread ended
	return 0;
}

//------------------------------------------------------------
// Worker 스레드 - NET 5바이트 암호화 헤더
// [MultiThread] 디버그 printf 제거
//------------------------------------------------------------
unsigned int __stdcall CNetServer::WorkerThread(LPVOID arg)
{
	CNetServer* pServer = (CNetServer*)arg;
	cSessionMap* sessionMap = pServer->_pSessionMap;

	while (1)
	{
		DWORD cbTransferred;
		SOCKETINFO* ptr;
		OVERLAPPED_CONTEXT* lpOverlapped;
		int retval = GetQueuedCompletionStatus(pServer->_hWorkerThreadIOCP, &cbTransferred, (PULONG_PTR)&ptr, (LPOVERLAPPED*)&lpOverlapped, INFINITE);

		// PQCS(0, 0, NULL) 종료 신호
		if (ptr == nullptr) break;

		if (cbTransferred == 0 || retval == 0)
		{
			// 정상 종료 / 클라 연결 끊김 - 로그 생략 (재접속 더미로 인한 폭증 방지)
			SessionKey origin = ptr->_sessionKey;
			if (sessionMap->DecreaseSessionIO(ptr) == ReleaseResult::Released)
			{
				pServer->_sessionCount.fetch_sub(1, std::memory_order_relaxed);
				pServer->OnClientLeave(origin);
			}
			continue;
		}

		if (lpOverlapped->op == ERecv)
		{
			CRingBuffer* rb = ptr->_recvBuf;
			if (rb->MoveRear(cbTransferred) == 0)
			{
				LOG(L"NetServer", CSystemLog::LEVEL_ERROR,
					L"[Session:%llu] RecvBuf overflow - Disconnect",
					ptr->_sessionKey.GetSessionId());
				pServer->Disconnect(ptr->_sessionKey);
			}
			else
			{
				// NET 패킷 파싱 (5바이트 암호화 헤더)
				while (rb->GetUseSize() >= dfPACKET_HEADERSIZE)
				{
					char tempHead[dfPACKET_HEADERSIZE];
					rb->Peek(tempHead, dfPACKET_HEADERSIZE);
					PacketHeader* header = (PacketHeader*)tempHead;

					// 패킷 코드 검증
					if (header->Code != dfPACKET_CODE)
					{
						InterlockedIncrement(&pServer->_invalidPacketCodeCount);
						pServer->Disconnect(ptr->_sessionKey);
						break;
					}

					if (header->Len == 0 || header->Len > 500)
					{
						InterlockedIncrement(&pServer->_invalidPacketLenCount);
						pServer->Disconnect(ptr->_sessionKey);
						break;
					}

					if (rb->GetUseSize() < dfPACKET_HEADERSIZE + header->Len)
						break;

					rb->MoveFront(dfPACKET_HEADERSIZE);

					char tempBuf[500];
					rb->Dequeue(tempBuf, header->Len);

					CPacket* contentPacket = CPacket::Alloc();
					contentPacket->PutData(tempBuf, header->Len);

					// NET 복호화
					if (!contentPacket->DecodeForNet(header, dfPACKET_KEY))
					{
						InterlockedIncrement(&pServer->_decodeForNetFailCount);
						contentPacket->SubRef();
						pServer->Disconnect(ptr->_sessionKey);
						break;
					}

					pServer->OnRecv(ptr->_sessionKey, contentPacket);
					contentPacket->SubRef();
				}

				pServer->WsaRecvSession(ptr);
			}

			SessionKey origin = ptr->_sessionKey;
			if (sessionMap->DecreaseSessionIO(ptr) == ReleaseResult::Released)
			{
				pServer->_sessionCount.fetch_sub(1, std::memory_order_relaxed);
				pServer->OnClientLeave(origin);
			}
		}
		else if (lpOverlapped->op == ESend)
		{
			ptr->_sendBuf->Lock();
			int sendPacketNum = ptr->_sendPacketNum;
			int srfront = ptr->_sendBuf->GetFront();
			int srCapacity = ptr->_sendBuf->GetBufferSize();
			CPacket** ppacket = ptr->_sendBuf->GetBufPtr();

			for (int i = 0; i < sendPacketNum; i++)
			{
				ppacket[srfront]->SubRef();
				srfront = (srfront + 1) % srCapacity;
			}
			ptr->_sendBuf->MoveFront(sendPacketNum);

			if (InterlockedCompareExchange(&ptr->_IsSending, 0, 1) != 1)
			{
				__debugbreak();
			}

			if (pServer->CanSend(ptr))
			{
				if (!pServer->SendPost(ptr))
				{
					InterlockedExchange(&ptr->_IsSending, 0);
					ptr->_sendBuf->UnLock();
					SessionKey origin = ptr->_sessionKey;
					if (sessionMap->DecreaseSessionIO(ptr) == ReleaseResult::Released)
					{
						pServer->_sessionCount.fetch_sub(1, std::memory_order_relaxed);
						pServer->OnClientLeave(origin);
					}
					continue;
				}
			}
			ptr->_sendBuf->UnLock();

			SessionKey origin = ptr->_sessionKey;
			if (sessionMap->DecreaseSessionIO(ptr) == ReleaseResult::Released)
			{
				pServer->_sessionCount.fetch_sub(1, std::memory_order_relaxed);
				pServer->OnClientLeave(origin);
			}
		}
		else
		{
			__debugbreak();
		}
	}

	return 0;
}

bool CNetServer::Disconnect(SessionKey sessionkey)
{
	SOCKETINFO* ptr = _pSessionMap->GetSessionptr(sessionkey);
	if (ptr == nullptr) return false;

	if (ptr->_sessionKey.GetSessionId() != sessionkey.GetSessionId())
	{
		// 재활용된 세션 - IOCount 증가분만 되돌림, Release 로직 금지
		InterlockedDecrement((unsigned long*)&ptr->_IOCount);
		return false;
	}

	shutdown(ptr->_sock, SD_BOTH);

	SessionKey origin = ptr->_sessionKey;
	if (_pSessionMap->DecreaseSessionIO(ptr) == ReleaseResult::Released)
	{
		_sessionCount.fetch_sub(1, std::memory_order_relaxed);
		OnClientLeave(origin);
	}
	return true;
}

bool CNetServer::SendPacket(SessionKey sessionkey, CPacket* cp)
{
	SOCKETINFO* ptr = _pSessionMap->GetSessionptr(sessionkey);
	if (ptr == nullptr) return false;

	if (ptr->_sessionKey.GetSessionId() != sessionkey.GetSessionId())
	{
		// 재활용된 세션 - IOCount 증가분만 되돌림, Release 로직 금지
		InterlockedDecrement((unsigned long*)&ptr->_IOCount);
		return false;
	}

	// NET 인코딩 (5바이트 암호화 헤더)
	cp->EncodeForNet(dfPACKET_CODE, dfPACKET_KEY);

	cp->AddRef();
	ptr->_sendBuf->Lock();
	int ret = ptr->_sendBuf->Enqueue(cp);
	if (!ret)
	{
		InterlockedIncrement(&_sendBufferFullCount);
		LOG(L"NetServer", CSystemLog::LEVEL_ERROR,
			L"[Session:%llu] SendBuf Full - Disconnect",
			ptr->_sessionKey.GetSessionId());
		ptr->_sendBuf->UnLock();
		cp->SubRef();
		shutdown(ptr->_sock, SD_BOTH);
		SessionKey origin = ptr->_sessionKey;
		if (_pSessionMap->DecreaseSessionIO(ptr) == ReleaseResult::Released)
		{
			_sessionCount.fetch_sub(1, std::memory_order_relaxed);
			OnClientLeave(origin);
		}
		return false;
	}

	if (!CanSend(ptr))
	{
		SessionKey origin = ptr->_sessionKey;
		if (_pSessionMap->DecreaseSessionIO(ptr) == ReleaseResult::Released)
		{
			_sessionCount.fetch_sub(1, std::memory_order_relaxed);
			OnClientLeave(origin);
		}
		ptr->_sendBuf->UnLock();
		return true;
	}

	if (!SendPost(ptr))
	{
		SessionKey origin = ptr->_sessionKey;
		if (_pSessionMap->DecreaseSessionIO(ptr) == ReleaseResult::Released)
		{
			_sessionCount.fetch_sub(1, std::memory_order_relaxed);
			OnClientLeave(origin);
		}
		ptr->_sendBuf->UnLock();
		return false;
	}

	SessionKey origin = ptr->_sessionKey;
	if (_pSessionMap->DecreaseSessionIO(ptr) == ReleaseResult::Released)
	{
		_sessionCount.fetch_sub(1, std::memory_order_relaxed);
		OnClientLeave(origin);
	}
	ptr->_sendBuf->UnLock();
	return true;
}

bool CNetServer::CanSend(SOCKETINFO* ptr)
{
	if (InterlockedCompareExchange(&ptr->_IsSending, 1, 0) != 0)
		return false;

	ptr->_sendBuf->Lock();
	if (ptr->_sendBuf->GetUseSize() == 0)
	{
		InterlockedExchange(&ptr->_IsSending, 0);
		ptr->_sendBuf->UnLock();
		return false;
	}
	ptr->_sendBuf->UnLock();
	return true;
}

bool CNetServer::SendPost(SOCKETINFO* ptr)
{
	CPacketRingBuffer* prb = ptr->_sendBuf;

	ptr->_sendOverlapped->op = ESend;
	ZeroMemory(ptr->_sendOverlapped, sizeof(OVERLAPPED));

	int remain = prb->GetUseSize();
	if (remain == 0) { __debugbreak(); }

	WSABUF wsabuf[dfSEND_WSABUF_MAX] = {};
	int bufIndex = 0;
	int rbFront = prb->GetFront();
	int rbCapacity = prb->GetBufferSize();
	CPacket** cpacket = prb->GetBufPtr();

	if (remain > dfSEND_WSABUF_MAX) remain = dfSEND_WSABUF_MAX;

	for (int i = 0; i < remain; i++)
	{
		CPacket* frontpacket = cpacket[rbFront];
		wsabuf[bufIndex].buf = frontpacket->GetBufferPtr();
		wsabuf[bufIndex].len = frontpacket->GetDataSize();
		bufIndex++;
		rbFront = (rbFront + 1) % rbCapacity;
	}

	if (!_pSessionMap->IncreaseSessionIO(ptr))
		return false;

	DWORD sendBytes = 0;
	ptr->_sendPacketNum = remain;
	int retval = WSASend(ptr->_sock, wsabuf, remain, &sendBytes, 0, (LPWSAOVERLAPPED)ptr->_sendOverlapped, NULL);

	if (retval == SOCKET_ERROR)
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			SessionKey origin = ptr->_sessionKey;
			if (_pSessionMap->DecreaseSessionIO(ptr) == ReleaseResult::Released)
			{
				_sessionCount.fetch_sub(1, std::memory_order_relaxed);
				OnClientLeave(origin);
			}
			return false;
		}
	}
	return true;
}

bool CNetServer::WsaRecvSession(SOCKETINFO* ptr)
{
	ptr->_recvOverlapped->op = ERecv;
	ZeroMemory(ptr->_recvOverlapped, sizeof(OVERLAPPED));

	int recvlen = ptr->_recvBuf->GetFreeSize();
	int retval;

	// 다음 Recv를 위한 IOCount 선증가 (실패 시 WSARecv 걸지 않음)
	if (!_pSessionMap->IncreaseSessionIO(ptr))
		return false;

	if (recvlen > ptr->_recvBuf->DirectEnqueueSize())
	{
		WSABUF wsabuf[2];
		wsabuf[0].buf = ptr->_recvBuf->GetRearBufferPtr();
		wsabuf[0].len = ptr->_recvBuf->DirectEnqueueSize();
		wsabuf[1].buf = ptr->_recvBuf->GetBufPtr();
		wsabuf[1].len = recvlen - ptr->_recvBuf->DirectEnqueueSize();
		DWORD recvbytes;
		DWORD flags = 0;
		retval = WSARecv(ptr->_sock, wsabuf, 2, &recvbytes, &flags, (LPWSAOVERLAPPED)ptr->_recvOverlapped, NULL);
	}
	else
	{
		WSABUF wsabuf;
		wsabuf.buf = ptr->_recvBuf->GetRearBufferPtr();
		wsabuf.len = ptr->_recvBuf->DirectEnqueueSize();
		DWORD recvbytes;
		DWORD flags = 0;
		retval = WSARecv(ptr->_sock, &wsabuf, 1, &recvbytes, &flags, (LPWSAOVERLAPPED)ptr->_recvOverlapped, NULL);
	}

	if (retval == SOCKET_ERROR)
	{
		int wsaErr = WSAGetLastError();
		if (wsaErr != WSA_IO_PENDING)
		{
			// 정상 종료성 에러는 로그 생략 (10054: ConnReset, 10053: ConnAborted, 64: NetnameDeleted, 1236: ConnectionAborted)
			if (wsaErr != WSAECONNRESET && wsaErr != WSAECONNABORTED &&
				wsaErr != ERROR_NETNAME_DELETED && wsaErr != ERROR_CONNECTION_ABORTED)
			{
				LOG(L"NetServer", CSystemLog::LEVEL_ERROR,
					L"[Session:%llu] WsaRecvSession WSARecv failed (err:%d)",
					ptr->_sessionKey.GetSessionId(), wsaErr);
			}
			SessionKey origin = ptr->_sessionKey;
			if (_pSessionMap->DecreaseSessionIO(ptr) == ReleaseResult::Released)
			{
				_sessionCount.fetch_sub(1, std::memory_order_relaxed);
				OnClientLeave(origin);
			}
			return false;
		}
	}
	return true;
}

unsigned int __stdcall CNetServer::MonitorThread(void* arg)
{
	CNetServer* server = reinterpret_cast<CNetServer*>(arg);
	while (server->_isRunning)
	{
		Sleep(1000);
		int count = server->_acceptCount.exchange(0, std::memory_order_relaxed);
		server->_acceptTPS.store(count, std::memory_order_relaxed);
	}
	return 0;
}
