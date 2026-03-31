#include "CMonitoringServer.h"
#include "CPacketForMultiThread.h"
#include "MonitorProtocol.h"
#include "CommonProtocol.h"
#include "CSystemLog.h"
#include "SystemMonitor.h"
#include <ws2tcpip.h>
#include <cstring>
#include <conio.h>

#define dfSERVER_NO_MACHINE 0

CMonitoringServer::CMonitoringServer()
	: _lanServer(this), _netServer(this)
{
	InitializeSRWLock(&_lanSessionLock);
	InitializeSRWLock(&_netSessionLock);
	memset((void*)_serverRecvCount, 0, sizeof(_serverRecvCount));
	memset((void*)_serverConnected, 0, sizeof(_serverConnected));
}

CMonitoringServer::~CMonitoringServer()
{
	Stop();
}

bool CMonitoringServer::Start()
{
	if (!_lanServer.Start(dfLAN_SERVER_PORT, dfLAN_SESSION_MAX))
	{
		printf("[MonitoringServer] LanServer Start failed\n");
		return false;
	}

	if (!_netServer.Start(dfNET_SERVER_PORT, dfNET_SESSION_MAX))
	{
		printf("[MonitoringServer] NetServer Start failed\n");
		return false;
	}

	if (!SystemMonitor::Initialize())
	{
		printf("[MonitoringServer] SystemMonitor Initialize failed\n");
		return false;
	}

	//pc데이터 송신 시작
	_bMonitorAlive = true;
	unsigned int tid;
	_hMonitorThread = (HANDLE)_beginthreadex(NULL, 0, MonitorThread, this, 0, &tid);

	// 화면 갱신 스레드 시작
	_bDisplayAlive = true;
	unsigned int tid2;
	_hDisplayThread = (HANDLE)_beginthreadex(NULL, 0, DisplayThread, this, 0, &tid2);

	return true;
}

void CMonitoringServer::Stop()
{
	_bDisplayAlive = false;
	if (_hDisplayThread)
	{
		WaitForSingleObject(_hDisplayThread, 3000);
		CloseHandle(_hDisplayThread);
		_hDisplayThread = NULL;
	}
	_lanServer.Stop();
	_netServer.Stop();
}

//=============================================================
// CLanServerImpl 콜백 (내부 서버 접속)
//=============================================================

bool CMonitoringServer::CLanServerImpl::OnConnectionRequest(std::string IP, int Port)
{
	return true;
}

void CMonitoringServer::CLanServerImpl::OnClientJoin(SOCKADDR_IN Client, SessionKey s)
{
}

void CMonitoringServer::CLanServerImpl::OnClientLeave(SessionKey s)
{
	int serverNo = -1;
	AcquireSRWLockExclusive(&_pOwner->_lanSessionLock);
	auto it = _pOwner->_lanSessionToServerNo.find(s.GetSessionId());
	if (it != _pOwner->_lanSessionToServerNo.end())
	{
		serverNo = it->second;
		_pOwner->_lanSessionToServerNo.erase(it);
	}
	ReleaseSRWLockExclusive(&_pOwner->_lanSessionLock);

	if (serverNo >= 0 && serverNo < dfMAX_SERVER_NO)
	{
		_pOwner->_serverConnected[serverNo] = false;
	}
}

void CMonitoringServer::CLanServerImpl::OnRecv(SessionKey s, CPacket* pPacket)
{
	WORD type;
	*pPacket >> type;

	switch (type)
	{
	case en_PACKET_SS_MONITOR_LOGIN:
		_pOwner->Handle_SS_MONITOR_LOGIN(s, pPacket);
		break;

	case en_PACKET_SS_MONITOR_DATA_UPDATE:
		_pOwner->Handle_SS_MONITOR_DATA_UPDATE(s, pPacket);
		break;

	default:
		LOG(L"MonitoringServer", CSystemLog::LEVEL_ERROR,
			L"[LAN] Unknown packet type: %d (Session:%llu)", type, s.GetSessionId());
		break;
	}
}

void CMonitoringServer::CLanServerImpl::OnError(int errorcode, const char* msg)
{
}

//=============================================================
// CNetServerImpl 콜백 (모니터링 툴 접속)
//=============================================================

bool CMonitoringServer::CNetServerImpl::OnConnectionRequest(std::string IP, int Port)
{
	return true;
}

void CMonitoringServer::CNetServerImpl::OnClientJoin(SOCKADDR_IN Client, SessionKey s)
{
}

void CMonitoringServer::CNetServerImpl::OnClientLeave(SessionKey s)
{
	AcquireSRWLockExclusive(&_pOwner->_netSessionLock);
	auto& v = _pOwner->_authedNetSessions;
	for (auto it = v.begin(); it != v.end(); ++it)
	{
		if (it->GetSessionId() == s.GetSessionId())
		{
			v.erase(it);
			break;
		}
	}
	ReleaseSRWLockExclusive(&_pOwner->_netSessionLock);
}

void CMonitoringServer::CNetServerImpl::OnRecv(SessionKey s, CPacket* pPacket)
{
	WORD type;
	*pPacket >> type;

	switch (type)
	{
	case en_PACKET_CS_MONITOR_TOOL_REQ_LOGIN:
		_pOwner->Handle_CS_MONITOR_TOOL_REQ_LOGIN(s, pPacket);
		break;

	default:
		LOG(L"MonitoringServer", CSystemLog::LEVEL_ERROR,
			L"[NET] Unknown packet type: %d (Session:%llu)", type, s.GetSessionId());
		break;
	}
}

void CMonitoringServer::CNetServerImpl::OnError(int errorcode, const char* msg)
{
}

//=============================================================
// SS 패킷 처리
//=============================================================

void CMonitoringServer::Handle_SS_MONITOR_LOGIN(SessionKey lanSession, CPacket* pPacket)
{
	int serverNo;
	*pPacket >> serverNo;

	AcquireSRWLockExclusive(&_lanSessionLock);
	_lanSessionToServerNo[lanSession.GetSessionId()] = serverNo;
	ReleaseSRWLockExclusive(&_lanSessionLock);

	if (serverNo >= 0 && serverNo < dfMAX_SERVER_NO)
	{
		_serverConnected[serverNo] = true;
	}
}

void CMonitoringServer::Handle_SS_MONITOR_DATA_UPDATE(SessionKey lanSession, CPacket* pPacket)
{
	BYTE dataType;
	int dataValue;
	int timeStamp;

	*pPacket >> dataType;
	*pPacket >> dataValue;
	*pPacket >> timeStamp;

	// LAN 세션에서 ServerNo 조회
	int serverNo = -1;
	AcquireSRWLockShared(&_lanSessionLock);
	auto it = _lanSessionToServerNo.find(lanSession.GetSessionId());
	if (it != _lanSessionToServerNo.end())
	{
		serverNo = it->second;
	}
	ReleaseSRWLockShared(&_lanSessionLock);

	if (serverNo == -1)
	{
		return;
	}

	InterlockedIncrement(&_lanRecvCount);
	if (serverNo >= 0 && serverNo < dfMAX_SERVER_NO)
	{
		InterlockedIncrement(&_serverRecvCount[serverNo]);
	}

	BroadcastToMonitorClients((BYTE)serverNo, dataType, dataValue, timeStamp);
}

//=============================================================
// CS 패킷 처리
//=============================================================

void CMonitoringServer::Handle_CS_MONITOR_TOOL_REQ_LOGIN(SessionKey netSession, CPacket* pPacket)
{
	char loginKey[dfLOGIN_KEY_LEN + 1] = {};
	pPacket->GetData(loginKey, dfLOGIN_KEY_LEN);

	BYTE loginResult;

	if (memcmp(loginKey, dfLOGIN_KEY, dfLOGIN_KEY_LEN) == 0)
	{
		loginResult = dfMONITOR_TOOL_LOGIN_OK;

		AcquireSRWLockExclusive(&_netSessionLock);
		_authedNetSessions.push_back(netSession);
		ReleaseSRWLockExclusive(&_netSessionLock);
	}
	else
	{
		loginResult = dfMONITOR_TOOL_LOGIN_ERR_SESSIONKEY;
	}

	CPacket* resPacket = CPacket::Alloc();
	resPacket->_MsgheaderSize = dfNET_HEADERSIZE;
	char dummy[dfNET_HEADERSIZE] = {};
	resPacket->PutData(dummy, dfNET_HEADERSIZE);
	WORD type = en_PACKET_CS_MONITOR_TOOL_RES_LOGIN;
	*resPacket << type;
	*resPacket << loginResult;

	_netServer.SendPacket(netSession, resPacket);
	resPacket->SubRef();
}

//=============================================================
// 모니터 데이터 브로드캐스트
//=============================================================

void CMonitoringServer::BroadcastToMonitorClients(BYTE serverNo, BYTE dataType, int dataValue, int timeStamp)
{
	AcquireSRWLockShared(&_netSessionLock);

	if (_authedNetSessions.empty())
	{
		ReleaseSRWLockShared(&_netSessionLock);
		return;
	}

	CPacket* pPacket = CPacket::Alloc();
	pPacket->_MsgheaderSize = dfNET_HEADERSIZE;
	char dummy[dfNET_HEADERSIZE] = {};
	pPacket->PutData(dummy, dfNET_HEADERSIZE);
	WORD type = en_PACKET_CS_MONITOR_TOOL_DATA_UPDATE;
	*pPacket << type;
	*pPacket << serverNo;
	*pPacket << dataType;
	*pPacket << dataValue;
	*pPacket << timeStamp;

	for (auto& session : _authedNetSessions)
	{
		pPacket->AddRef();
		if (_netServer.SendPacket(session, pPacket))
		{
			InterlockedIncrement(&_netBroadcastCount);
		}
		else
		{
			InterlockedIncrement(&_netSendFailCount);
		}
	}

	ReleaseSRWLockShared(&_netSessionLock);
	pPacket->SubRef();
}

//=============================================================
// 화면 갱신 스레드 (1초마다 콘솔 전체 갱신)
//=============================================================

unsigned int __stdcall CMonitoringServer::DisplayThread(LPVOID arg)
{
	CMonitoringServer* pServer = (CMonitoringServer*)arg;

	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

	// 커서 숨기기
	CONSOLE_CURSOR_INFO cursorInfo;
	GetConsoleCursorInfo(hConsole, &cursorInfo);
	cursorInfo.bVisible = FALSE;
	SetConsoleCursorInfo(hConsole, &cursorInfo);

	const int LINE_WIDTH = 70;
	char buf[4096];

	while (pServer->_bDisplayAlive)
	{
		Sleep(1000);

		// 카운터 스냅샷 (초당 값)
		long lanRecv = InterlockedExchange(&pServer->_lanRecvCount, 0);
		long netBroadcast = InterlockedExchange(&pServer->_netBroadcastCount, 0);
		long netFail = InterlockedExchange(&pServer->_netSendFailCount, 0);

		int lanSessionCount = pServer->_lanServer.GetSessionCount();
		int netSessionCount = pServer->_netServer.GetSessionCount();

		int authedCount = 0;
		AcquireSRWLockShared(&pServer->_netSessionLock);
		authedCount = (int)pServer->_authedNetSessions.size();
		ReleaseSRWLockShared(&pServer->_netSessionLock);

		// 서버별 초당 수신량 스냅샷
		long serverRecv[dfMAX_SERVER_NO];
		for (int i = 0; i < dfMAX_SERVER_NO; i++)
		{
			serverRecv[i] = InterlockedExchange(&pServer->_serverRecvCount[i], 0);
		}

		// 화면 버퍼 생성
		int pos = 0;

		pos += sprintf_s(buf + pos, sizeof(buf) - pos,
			"%-*s\n", LINE_WIDTH,
			"=== Monitoring Server ===");
		pos += sprintf_s(buf + pos, sizeof(buf) - pos,
			"%-*s\n", LINE_WIDTH,
			"  Press 'q' to quit");
		pos += sprintf_s(buf + pos, sizeof(buf) - pos,
			"%-*s\n", LINE_WIDTH,
			"----------------------------------------------------------------------");

		// 접속 현황
		char line[128];
		sprintf_s(line, sizeof(line),
			"  LAN Servers: %d   |   NET Clients: %d (Auth: %d)",
			lanSessionCount, netSessionCount, authedCount);
		pos += sprintf_s(buf + pos, sizeof(buf) - pos, "%-*s\n", LINE_WIDTH, line);

		pos += sprintf_s(buf + pos, sizeof(buf) - pos,
			"%-*s\n", LINE_WIDTH,
			"----------------------------------------------------------------------");

		// 서버별 상태
		pos += sprintf_s(buf + pos, sizeof(buf) - pos,
			"%-*s\n", LINE_WIDTH,
			"  [Connected Servers]");

		bool anyServer = false;
		for (int i = 0; i < dfMAX_SERVER_NO; i++)
		{
			if (pServer->_serverConnected[i])
			{
				anyServer = true;
				sprintf_s(line, sizeof(line),
					"    Server #%-3d   Recv/s: %ld", i, serverRecv[i]);
				pos += sprintf_s(buf + pos, sizeof(buf) - pos, "%-*s\n", LINE_WIDTH, line);
			}
		}
		if (!anyServer)
		{
			pos += sprintf_s(buf + pos, sizeof(buf) - pos,
				"%-*s\n", LINE_WIDTH,
				"    (none)");
		}

		pos += sprintf_s(buf + pos, sizeof(buf) - pos,
			"%-*s\n", LINE_WIDTH,
			"----------------------------------------------------------------------");

		// 전체 통계
		sprintf_s(line, sizeof(line),
			"  LAN Recv/s: %ld", lanRecv);
		pos += sprintf_s(buf + pos, sizeof(buf) - pos, "%-*s\n", LINE_WIDTH, line);

		sprintf_s(line, sizeof(line),
			"  NET Send/s: %ld   |   NET Fail/s: %ld", netBroadcast, netFail);
		pos += sprintf_s(buf + pos, sizeof(buf) - pos, "%-*s\n", LINE_WIDTH, line);

		pos += sprintf_s(buf + pos, sizeof(buf) - pos,
			"%-*s\n", LINE_WIDTH,
			"----------------------------------------------------------------------");

		// 빈 줄로 나머지 채우기 (이전 출력 잔상 제거)
		for (int i = 0; i < 5; i++)
		{
			pos += sprintf_s(buf + pos, sizeof(buf) - pos, "%-*s\n", LINE_WIDTH, "");
		}

		// 콘솔에 한번에 출력
		COORD origin = { 0, 0 };
		SetConsoleCursorPosition(hConsole, origin);
		DWORD written;
		WriteConsoleA(hConsole, buf, pos, &written, NULL);
	}

	// 커서 복원
	cursorInfo.bVisible = TRUE;
	SetConsoleCursorInfo(hConsole, &cursorInfo);

	return 0;
}

unsigned int __stdcall CMonitoringServer::MonitorThread(LPVOID arg)
{
	CMonitoringServer* pServer = (CMonitoringServer*)arg;

	const BYTE MACHINE_NO = dfSERVER_NO_MACHINE;

	while (pServer->_bMonitorAlive)
	{
		Sleep(1000);

		// 1️ 시스템 상태 갱신
		SystemMonitor::Update();

		// 2️ 값 가져오기
		int cpu = SystemMonitor::GetCpuTotal();
		int nonPaged = SystemMonitor::GetNonPagedMemory();
		int netRecv = SystemMonitor::GetNetworkRecvBytes();
		int netSend = SystemMonitor::GetNetworkSendBytes();
		int availMem = SystemMonitor::GetAvailableMemory();

		int timeStamp = GetTickCount64();

		// 3️ 브로드캐스트
		pServer->BroadcastToMonitorClients(MACHINE_NO, dfMONITOR_DATA_TYPE_MONITOR_CPU_TOTAL, cpu, timeStamp);
		pServer->BroadcastToMonitorClients(MACHINE_NO, dfMONITOR_DATA_TYPE_MONITOR_NONPAGED_MEMORY, nonPaged, timeStamp);
		pServer->BroadcastToMonitorClients(MACHINE_NO, dfMONITOR_DATA_TYPE_MONITOR_NETWORK_RECV, netRecv, timeStamp);
		pServer->BroadcastToMonitorClients(MACHINE_NO, dfMONITOR_DATA_TYPE_MONITOR_NETWORK_SEND, netSend, timeStamp);
		pServer->BroadcastToMonitorClients(MACHINE_NO, dfMONITOR_DATA_TYPE_MONITOR_AVAILABLE_MEMORY, availMem, timeStamp);
	}

	return 0;
}
