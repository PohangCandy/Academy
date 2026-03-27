#include "CMonitoringServer.h"
#include "CPacketForMultiThread.h"
#include "MonitorProtocol.h"
#include "CommonProtocol.h"
#include "CSystemLog.h"
#include <ws2tcpip.h>
#include <cstring>

CMonitoringServer::CMonitoringServer()
	: _lanServer(this), _netServer(this)
{
	InitializeSRWLock(&_lanSessionLock);
	InitializeSRWLock(&_netSessionLock);
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

	printf("[MonitoringServer] Started (LAN:%d, NET:%d)\n", dfLAN_SERVER_PORT, dfNET_SERVER_PORT);
	return true;
}

void CMonitoringServer::Stop()
{
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
	char ipStr[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &Client.sin_addr, ipStr, sizeof(ipStr));
	printf("[LAN] Server connected: %s:%d (Session:%llu)\n", ipStr, ntohs(Client.sin_port), s.GetSessionId());
}

void CMonitoringServer::CLanServerImpl::OnClientLeave(SessionKey s)
{
	printf("[LAN] Server disconnected (Session:%llu)\n", s.GetSessionId());

	AcquireSRWLockExclusive(&_pOwner->_lanSessionLock);
	_pOwner->_lanSessionToServerNo.erase(s.GetSessionId());
	ReleaseSRWLockExclusive(&_pOwner->_lanSessionLock);
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
	printf("[LAN Error] %d: %s\n", errorcode, msg);
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
	char ipStr[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &Client.sin_addr, ipStr, sizeof(ipStr));
	printf("[NET] Monitor client connected: %s:%d (Session:%llu)\n", ipStr, ntohs(Client.sin_port), s.GetSessionId());
}

void CMonitoringServer::CNetServerImpl::OnClientLeave(SessionKey s)
{
	printf("[NET] Monitor client disconnected (Session:%llu)\n", s.GetSessionId());

	// 인증 목록에서 제거
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
	printf("[NET Error] %d: %s\n", errorcode, msg);
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

	printf("[LAN] Server login: ServerNo=%d (Session:%llu)\n", serverNo, lanSession.GetSessionId());
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
		LOG(L"MonitoringServer", CSystemLog::LEVEL_ERROR,
			L"[LAN] Data update from unregistered session:%llu", lanSession.GetSessionId());
		return;
	}

	// 인증된 모니터링 클라이언트에 브로드캐스트
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

		// 인증 목록에 추가
		AcquireSRWLockExclusive(&_netSessionLock);
		_authedNetSessions.push_back(netSession);
		ReleaseSRWLockExclusive(&_netSessionLock);

		printf("[NET] Monitor client login OK (Session:%llu)\n", netSession.GetSessionId());
	}
	else
	{
		loginResult = dfMONITOR_TOOL_LOGIN_ERR_SESSIONKEY;
		printf("[NET] Monitor client login FAILED (Session:%llu)\n", netSession.GetSessionId());
	}

	// 로그인 응답 전송
	CPacket* resPacket = CPacket::Alloc();
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
	WORD type = en_PACKET_CS_MONITOR_TOOL_DATA_UPDATE;
	*pPacket << type;
	*pPacket << serverNo;
	*pPacket << dataType;
	*pPacket << dataValue;
	*pPacket << timeStamp;

	for (auto& session : _authedNetSessions)
	{
		pPacket->AddRef();
		_netServer.SendPacket(session, pPacket);
	}

	ReleaseSRWLockShared(&_netSessionLock);
	pPacket->SubRef();
}
