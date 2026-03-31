#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma comment(lib, "ws2_32")

#include "CLanServer.h"
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

CLanServer::CLanServer()
	: _hWorkerThreadIOCP(NULL), _listenSock(INVALID_SOCKET), _pSessionMap(nullptr)
{
}

CLanServer::~CLanServer()
{
	Stop();
}

//------------------------------------------------------------
// [변경] Accept를 별도 스레드로 분리 → Start()가 즉시 반환
// [변경] 자체 cSessionMap 인스턴스 생성
//------------------------------------------------------------
bool CLanServer::Start(int port, int maxSession)
{
	int retval;

	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return false;

	_pSessionMap = new cSessionMap(maxSession);

	_hWorkerThreadIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (_hWorkerThreadIOCP == NULL) return false;

	SYSTEM_INFO si;
	GetSystemInfo(&si);

	HANDLE hThread = NULL;
	unsigned int uiThreadID;

	for (int i = 0; i < (int)si.dwNumberOfProcessors * 2; i++)
	{
		hThread = (HANDLE)_beginthreadex(NULL, 0, WorkerThread, this, 0, &uiThreadID);
		if (hThread == NULL) return false;
		CloseHandle(hThread);
	}

	// 모니터 스레드
	hThread = (HANDLE)_beginthreadex(NULL, 0, MonitorThread, this, 0, &uiThreadID);
	if (hThread == NULL) return false;
	CloseHandle(hThread);

	// Listen 소켓
	_listenSock = socket(AF_INET, SOCK_STREAM, 0);
	if (_listenSock == INVALID_SOCKET) { err_quit("LanServer socket()"); return false; }

	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(port);
	retval = bind(_listenSock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) { err_quit("LanServer bind()"); return false; }

	retval = listen(_listenSock, SOMAXCONN);
	if (retval == SOCKET_ERROR) { err_quit("LanServer listen()"); return false; }

	printf("[LanServer] Listening on port %d\n", port);

	// Accept 스레드 (별도 스레드로 분리)
	hThread = (HANDLE)_beginthreadex(NULL, 0, AcceptThread, this, 0, &uiThreadID);
	if (hThread == NULL) return false;
	CloseHandle(hThread);

	return true;
}

void CLanServer::Stop()
{
	if (_listenSock != INVALID_SOCKET)
	{
		closesocket(_listenSock);
		_listenSock = INVALID_SOCKET;
	}
	if (_pSessionMap != nullptr)
	{
		delete _pSessionMap;
		_pSessionMap = nullptr;
	}
}

int CLanServer::GetSessionCount()
{
	return _sessionCount;
}

//------------------------------------------------------------
// Accept 스레드 (기존 Start() 내부 while 루프를 스레드로 분리)
//------------------------------------------------------------
unsigned int __stdcall CLanServer::AcceptThread(LPVOID arg)
{
	CLanServer* pServer = (CLanServer*)arg;

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

		if (!pServer->_pSessionMap->IncreaseSessionIO(ptr))
		{
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
				SessionKey origin = ptr->_sessionKey;
				if (pServer->_pSessionMap->DecreaseSessionIO(ptr) == ReleaseResult::Released)
				{
					pServer->_sessionCount.fetch_sub(1, std::memory_order_relaxed);
					pServer->OnClientLeave(origin);
				}
			}
		}
	}

	printf("[LanServer] AcceptThread ended\n");
	return 0;
}

//------------------------------------------------------------
// Worker 스레드 - LAN용 (단순 WORD Len 헤더, 암호화 없음)
//------------------------------------------------------------
unsigned int __stdcall CLanServer::WorkerThread(LPVOID arg)
{
	CLanServer* pServer = (CLanServer*)arg;
	cSessionMap* sessionMap = pServer->_pSessionMap;

	while (1)
	{
		DWORD cbTransferred;
		SOCKETINFO* ptr;
		OVERLAPPED_CONTEXT* lpOverlapped;
		int retval = GetQueuedCompletionStatus(pServer->_hWorkerThreadIOCP, &cbTransferred, (PULONG_PTR)&ptr, (LPOVERLAPPED*)&lpOverlapped, INFINITE);

		if (ptr == nullptr) continue;

		if (cbTransferred == 0 || retval == 0)
		{
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
				printf("[LanServer] Recv buffer full\n");
				__debugbreak();
			}

			// NET 패킷 파싱 (5바이트 암호화 헤더)
			printf("[WorkerThread] ERecv: %d bytes, session:%llu\n", cbTransferred, ptr->_sessionKey.GetSessionId());
			while (rb->GetUseSize() >= dfPACKET_HEADERSIZE)
			{
				char tempHead[dfPACKET_HEADERSIZE];
				rb->Peek(tempHead, dfPACKET_HEADERSIZE);
				PacketHeader* header = (PacketHeader*)tempHead;

				printf("[WorkerThread] Header: Code=0x%02X, Len=%d, RandKey=%d\n", header->Code, header->Len, header->RandKey);

				// 패킷 코드 검증
				if (header->Code != dfPACKET_CODE)
				{
					LOG(L"LanServer", CSystemLog::LEVEL_ERROR,
						L"[Session:%llu] Invalid PacketCode: 0x%02X (expected: 0x%02X)",
						ptr->_sessionKey.GetSessionId(), header->Code, dfPACKET_CODE);
					pServer->Disconnect(ptr->_sessionKey);
					break;
				}

				if (header->Len > 500)
				{
					LOG(L"LanServer", CSystemLog::LEVEL_ERROR,
						L"[Session:%llu] Oversized Packet Len: %d",
						ptr->_sessionKey.GetSessionId(), header->Len);
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
				printf("[WorkerThread] Decoding payload %d bytes...\n", header->Len);
				if (!contentPacket->DecodeForNet(header, dfPACKET_KEY))
				{
					LOG(L"LanServer", CSystemLog::LEVEL_ERROR,
						L"[Session:%llu] DecodeForNet failed (checksum mismatch)",
						ptr->_sessionKey.GetSessionId());
					contentPacket->SubRef();
					pServer->Disconnect(ptr->_sessionKey);
					break;
				}

				printf("[WorkerThread] Decode OK, forwarding to OnRecv\n");
				contentPacket->AddRef();
				pServer->OnRecv(ptr->_sessionKey, contentPacket);
				contentPacket->SubRef();
			}

			if (!pServer->WsaRecvSession(ptr))
				continue;

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
			printf("[LanServer] Unknown overlapped op\n");
			__debugbreak();
		}
	}

	return 0;
}

bool CLanServer::Disconnect(SessionKey sessionkey)
{
	SOCKETINFO* ptr = _pSessionMap->GetSessionptr(sessionkey);
	if (ptr == nullptr) return false;

	if (ptr->_sessionKey.GetSessionId() != sessionkey.GetSessionId())
	{
		SessionKey origin = ptr->_sessionKey;
		if (_pSessionMap->DecreaseSessionIO(ptr) == ReleaseResult::Released)
		{
			_sessionCount.fetch_sub(1, std::memory_order_relaxed);
			OnClientLeave(origin);
		}
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

bool CLanServer::SendPacket(SessionKey sessionkey, CPacket* cp)
{
	SOCKETINFO* ptr = _pSessionMap->GetSessionptr(sessionkey);
	if (ptr == nullptr) return false;

	if (ptr->_sessionKey.GetSessionId() != sessionkey.GetSessionId())
	{
		SessionKey origin = ptr->_sessionKey;
		if (_pSessionMap->DecreaseSessionIO(ptr) == ReleaseResult::Released)
		{
			_sessionCount.fetch_sub(1, std::memory_order_relaxed);
			OnClientLeave(origin);
		}
		return false;
	}

	// NET 인코딩 (5바이트 암호화 헤더)
	cp->EncodeForNet(dfPACKET_CODE, dfPACKET_KEY);

	cp->AddRef();
	ptr->_sendBuf->Lock();
	int ret = ptr->_sendBuf->Enqueue(cp);
	if (!ret)
	{
		printf("[LanServer] SendBuf Enqueue failed\n");
		__debugbreak();
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

bool CLanServer::CanSend(SOCKETINFO* ptr)
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

bool CLanServer::SendPost(SOCKETINFO* ptr)
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

	if (remain > dfSEND_WSABUF_MAX) { __debugbreak(); }

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
				return false;
			}
		}
	}
	return true;
}

bool CLanServer::WsaRecvSession(SOCKETINFO* ptr)
{
	ptr->_recvOverlapped->op = ERecv;
	ZeroMemory(ptr->_recvOverlapped, sizeof(OVERLAPPED));

	int recvlen = ptr->_recvBuf->GetFreeSize();
	int retval;

	if (recvlen > ptr->_recvBuf->DirectEnqueueSize())
	{
		WSABUF wsabuf[2];
		wsabuf[0].buf = ptr->_recvBuf->GetRearBufferPtr();
		wsabuf[0].len = ptr->_recvBuf->DirectEnqueueSize();
		wsabuf[1].buf = ptr->_recvBuf->GetBufPtr();
		wsabuf[1].len = recvlen - ptr->_recvBuf->DirectEnqueueSize();
		DWORD recvbytes;
		DWORD flags = 0;
		if (!_pSessionMap->IncreaseSessionIO(ptr)) { __debugbreak(); }
		retval = WSARecv(ptr->_sock, wsabuf, 2, &recvbytes, &flags, (LPWSAOVERLAPPED)ptr->_recvOverlapped, NULL);
	}
	else
	{
		WSABUF wsabuf;
		wsabuf.buf = ptr->_recvBuf->GetRearBufferPtr();
		wsabuf.len = ptr->_recvBuf->DirectEnqueueSize();
		DWORD recvbytes;
		DWORD flags = 0;
		if (!_pSessionMap->IncreaseSessionIO(ptr)) { __debugbreak(); }
		retval = WSARecv(ptr->_sock, &wsabuf, 1, &recvbytes, &flags, (LPWSAOVERLAPPED)ptr->_recvOverlapped, NULL);
	}

	if (retval == SOCKET_ERROR)
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			SessionKey origin = ptr->_sessionKey;
			if (_pSessionMap->DecreaseSessionIO(ptr) == ReleaseResult::Released)
			{
				_sessionCount.fetch_sub(1, std::memory_order_relaxed);
				OnClientLeave(origin);
				return false;
			}
		}
	}
	return true;
}

unsigned int __stdcall CLanServer::MonitorThread(void* arg)
{
	CLanServer* server = reinterpret_cast<CLanServer*>(arg);
	while (1)
	{
		Sleep(1000);
		int count = server->_acceptCount.exchange(0, std::memory_order_relaxed);
		server->_acceptTPS.store(count, std::memory_order_relaxed);
	}
	return 0;
}
