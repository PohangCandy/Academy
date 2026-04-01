#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma comment(lib, "ws2_32")

#include "CLanClient.h"
#include "CRingBuffer.h"
#include "CPacketRingBuffer.h"
#include "OVERLAPPED_CONTEXT.h"
#include "CPacketForMultiThread.h"
#include "CSystemLog.h"
#include "errlog.h"
#include <ws2tcpip.h>

// LAN 헤더 (모니터링 서버의 CommonProtocol.h과 동일)
#define dfLAN_HEADERSIZE	(2)
#define dfSEND_WSABUF_MAX	(128)

#pragma pack(push, 1)
struct LanClientPacketHeader
{
	unsigned short Len;
};
#pragma pack(pop)

CLanClient::CLanClient()
	: _hIOCP(NULL), _sock(INVALID_SOCKET),
	_recvBuf(nullptr), _sendBuf(nullptr),
	_recvOverlapped(nullptr), _sendOverlapped(nullptr),
	_IsSending(0), _sendPacketNum(0),
	_IOCount(0), _bConnected(false)
{
}

CLanClient::~CLanClient()
{
	Disconnect();
}

bool CLanClient::Connect(const char* serverIP, int serverPort, int workerThreadCount, bool bNagle)
{
	if (_bConnected) return false;

	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return false;

	_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (_sock == INVALID_SOCKET)
	{
		err_quit("CLanClient socket()");
		return false;
	}

	// Nagle 옵션
	if (!bNagle)
	{
		BOOL opt = TRUE;
		setsockopt(_sock, IPPROTO_TCP, TCP_NODELAY, (char*)&opt, sizeof(opt));
	}

	// Linger
	LINGER optval;
	optval.l_onoff = 1;
	optval.l_linger = 0;
	setsockopt(_sock, SOL_SOCKET, SO_LINGER, (char*)&optval, sizeof(optval));

	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	inet_pton(AF_INET, serverIP, &serveraddr.sin_addr);
	serveraddr.sin_port = htons(serverPort);

	int retval = connect(_sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR)
	{
		printf("[CLanClient] Connect failed to %s:%d (err:%d)\n", serverIP, serverPort, WSAGetLastError());
		closesocket(_sock);
		_sock = INVALID_SOCKET;
		return false;
	}

	// 버퍼 할당
	_recvBuf = new CRingBuffer(4096);
	_sendBuf = new CPacketRingBuffer(256);
	_recvOverlapped = new OVERLAPPED_CONTEXT();
	_sendOverlapped = new OVERLAPPED_CONTEXT();

	_IsSending = 0;
	_sendPacketNum = 0;
	_IOCount = 0;
	_bConnected = true;

	// IOCP 생성 및 소켓 연결
	_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (_hIOCP == NULL)
	{
		Disconnect();
		return false;
	}

	CreateIoCompletionPort((HANDLE)_sock, _hIOCP, (ULONG_PTR)this, 0);

	// 워커 스레드
	for (int i = 0; i < workerThreadCount; i++)
	{
		unsigned int tid;
		HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, WorkerThread, this, 0, &tid);
		if (hThread == NULL)
		{
			Disconnect();
			return false;
		}
		CloseHandle(hThread);
	}

	printf("[CLanClient] Connected to %s:%d\n", serverIP, serverPort);

	// 첫 Recv 걸기 (OnEnterJoinServer에서 Send를 걸 수 있으므로,
	// Send 완료 시 IOCount가 0이 되는 것을 방지하기 위해 먼저 Recv를 건다)
	InterlockedIncrement(&_IOCount);
	if (!WsaRecvPost())
	{
		if (InterlockedDecrement(&_IOCount) == 0)
		{
			Release();
		}
		return false;
	}

	// 콜백
	OnEnterJoinServer();

	return true;
}

bool CLanClient::Disconnect()
{
	if (!_bConnected) return false;

	_bConnected = false;

	if (_sock != INVALID_SOCKET)
	{
		shutdown(_sock, SD_BOTH);
		closesocket(_sock);
		_sock = INVALID_SOCKET;
	}

	return true;
}

void CLanClient::Release()
{
	_bConnected = false;

	if (_sock != INVALID_SOCKET)
	{
		closesocket(_sock);
		_sock = INVALID_SOCKET;
	}

	// 잔여 Send 패킷 정리
	if (_sendBuf != nullptr)
	{
		_sendBuf->Lock();
		int remain = _sendBuf->GetUseSize();
		int front = _sendBuf->GetFront();
		int cap = _sendBuf->GetBufferSize();
		CPacket** buf = _sendBuf->GetBufPtr();
		for (int i = 0; i < remain; i++)
		{
			buf[front]->SubRef();
			front = (front + 1) % cap;
		}
		_sendBuf->MoveFront(remain);
		_sendBuf->UnLock();
	}

	delete _recvBuf; _recvBuf = nullptr;
	delete _sendBuf; _sendBuf = nullptr;
	delete _recvOverlapped; _recvOverlapped = nullptr;
	delete _sendOverlapped; _sendOverlapped = nullptr;

	OnLeaveServer();
}

bool CLanClient::SendPacket(CPacket* cp)
{
	if (!_bConnected) return false;

	// LAN 헤더 직접 쓰기 (WORD Len, 암호화 없음)
	if (!cp->IsEncoded)
	{
		int payloadSize = cp->GetDataSize() - dfLAN_HEADERSIZE;
		*(unsigned short*)cp->GetBufferPtr() = (unsigned short)payloadSize;
		cp->IsEncoded = true;
	}

	cp->AddRef();

	_sendBuf->Lock();
	int ret = _sendBuf->Enqueue(cp);
	if (!ret)
	{
		printf("[CLanClient] SendBuf Enqueue failed\n");
		_sendBuf->UnLock();
		cp->SubRef();
		return false;
	}
	_sendBuf->UnLock();

	if (!CanSend())
		return true;

	InterlockedIncrement(&_IOCount);
	if (!SendPost())
	{
		InterlockedExchange(&_IsSending, 0);
		if (InterlockedDecrement(&_IOCount) == 0)
		{
			Release();
		}
		return false;
	}

	return true;
}

bool CLanClient::CanSend()
{
	if (InterlockedCompareExchange(&_IsSending, 1, 0) != 0)
		return false;

	_sendBuf->Lock();
	if (_sendBuf->GetUseSize() == 0)
	{
		InterlockedExchange(&_IsSending, 0);
		_sendBuf->UnLock();
		return false;
	}
	_sendBuf->UnLock();
	return true;
}

bool CLanClient::SendPost()
{
	CPacketRingBuffer* prb = _sendBuf;

	_sendOverlapped->op = ESend;
	ZeroMemory(_sendOverlapped, sizeof(OVERLAPPED));

	prb->Lock();
	int remain = prb->GetUseSize();
	if (remain == 0)
	{
		prb->UnLock();
		return false;
	}

	WSABUF wsabuf[dfSEND_WSABUF_MAX] = {};
	int rbFront = prb->GetFront();
	int rbCapacity = prb->GetBufferSize();
	CPacket** cpacket = prb->GetBufPtr();

	if (remain > dfSEND_WSABUF_MAX) remain = dfSEND_WSABUF_MAX;

	for (int i = 0; i < remain; i++)
	{
		CPacket* frontpacket = cpacket[rbFront];
		wsabuf[i].buf = frontpacket->GetBufferPtr();
		wsabuf[i].len = frontpacket->GetDataSize();
		rbFront = (rbFront + 1) % rbCapacity;
	}
	prb->UnLock();

	_sendPacketNum = remain;
	DWORD sendBytes = 0;
	int retval = WSASend(_sock, wsabuf, remain, &sendBytes, 0, (LPWSAOVERLAPPED)_sendOverlapped, NULL);

	if (retval == SOCKET_ERROR)
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			return false;
		}
	}
	return true;
}

bool CLanClient::WsaRecvPost()
{
	_recvOverlapped->op = ERecv;
	ZeroMemory(_recvOverlapped, sizeof(OVERLAPPED));

	int freeSize = _recvBuf->GetFreeSize();
	int retval;

	if (freeSize > _recvBuf->DirectEnqueueSize())
	{
		WSABUF wsabuf[2];
		wsabuf[0].buf = _recvBuf->GetRearBufferPtr();
		wsabuf[0].len = _recvBuf->DirectEnqueueSize();
		wsabuf[1].buf = _recvBuf->GetBufPtr();
		wsabuf[1].len = freeSize - _recvBuf->DirectEnqueueSize();
		DWORD recvbytes;
		DWORD flags = 0;
		retval = WSARecv(_sock, wsabuf, 2, &recvbytes, &flags, (LPWSAOVERLAPPED)_recvOverlapped, NULL);
	}
	else
	{
		WSABUF wsabuf;
		wsabuf.buf = _recvBuf->GetRearBufferPtr();
		wsabuf.len = _recvBuf->DirectEnqueueSize();
		DWORD recvbytes;
		DWORD flags = 0;
		retval = WSARecv(_sock, &wsabuf, 1, &recvbytes, &flags, (LPWSAOVERLAPPED)_recvOverlapped, NULL);
	}

	if (retval == SOCKET_ERROR)
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			return false;
		}
	}
	return true;
}

unsigned int __stdcall CLanClient::WorkerThread(LPVOID arg)
{
	CLanClient* pClient = (CLanClient*)arg;

	while (1)
	{
		DWORD cbTransferred;
		ULONG_PTR completionKey;
		OVERLAPPED_CONTEXT* lpOverlapped;
		int retval = GetQueuedCompletionStatus(pClient->_hIOCP, &cbTransferred, &completionKey, (LPOVERLAPPED*)&lpOverlapped, INFINITE);

		// 종료 신호 (cbTransferred == 0 && completionKey == 0 && lpOverlapped == nullptr)
		if (cbTransferred == 0 && completionKey == 0 && lpOverlapped == nullptr)
			break;

		if (cbTransferred == 0 || retval == 0)
		{
			// 연결 끊김
			if (InterlockedDecrement(&pClient->_IOCount) == 0)
			{
				pClient->Release();
			}
			continue;
		}

		if (lpOverlapped->op == ERecv)
		{
			CRingBuffer* rb = pClient->_recvBuf;
			if (rb->MoveRear(cbTransferred) == 0)
			{
				printf("[CLanClient] Recv buffer full\n");
				__debugbreak();
			}

			// LAN 패킷 파싱 (WORD Len 헤더)
			while (rb->GetUseSize() >= dfLAN_HEADERSIZE)
			{
				char tempHead[dfLAN_HEADERSIZE];
				rb->Peek(tempHead, dfLAN_HEADERSIZE);
				LanClientPacketHeader* header = (LanClientPacketHeader*)tempHead;

				if (header->Len > 500)
				{
					LOG(L"CLanClient", CSystemLog::LEVEL_ERROR,
						L"Oversized Packet Len: %d", header->Len);
					pClient->Disconnect();
					break;
				}

				if (rb->GetUseSize() < dfLAN_HEADERSIZE + header->Len)
					break;

				rb->MoveFront(dfLAN_HEADERSIZE);

				char tempBuf[500];
				rb->Dequeue(tempBuf, header->Len);

				CPacket* contentPacket = CPacket::Alloc();
				contentPacket->PutData(tempBuf, header->Len);

				contentPacket->AddRef();
				pClient->OnRecv(contentPacket);
				contentPacket->SubRef();
			}

			// 다음 Recv 걸기
			InterlockedIncrement(&pClient->_IOCount);
			if (!pClient->WsaRecvPost())
			{
				if (InterlockedDecrement(&pClient->_IOCount) == 0)
				{
					pClient->Release();
				}
			}

			// 현재 IO 완료
			if (InterlockedDecrement(&pClient->_IOCount) == 0)
			{
				pClient->Release();
			}
		}
		else if (lpOverlapped->op == ESend)
		{
			// Send 완료 처리 - 보낸 패킷 SubRef
			pClient->_sendBuf->Lock();
			int sendPacketNum = pClient->_sendPacketNum;
			int srfront = pClient->_sendBuf->GetFront();
			int srCapacity = pClient->_sendBuf->GetBufferSize();
			CPacket** ppacket = pClient->_sendBuf->GetBufPtr();

			for (int i = 0; i < sendPacketNum; i++)
			{
				ppacket[srfront]->SubRef();
				srfront = (srfront + 1) % srCapacity;
			}
			pClient->_sendBuf->MoveFront(sendPacketNum);

			if (InterlockedCompareExchange(&pClient->_IsSending, 0, 1) != 1)
			{
				__debugbreak();
			}

			// 추가로 보낼 데이터가 있으면 SendPost
			if (pClient->CanSend())
			{
				InterlockedIncrement(&pClient->_IOCount);
				if (!pClient->SendPost())
				{
					InterlockedExchange(&pClient->_IsSending, 0);
					pClient->_sendBuf->UnLock();
					if (InterlockedDecrement(&pClient->_IOCount) == 0)
					{
						pClient->Release();
					}
					continue;
				}
			}
			pClient->_sendBuf->UnLock();

			if (InterlockedDecrement(&pClient->_IOCount) == 0)
			{
				pClient->Release();
			}
		}
	}

	return 0;
}
