#include "CMonitorClient.h"
#include "CPacketForMultiThread.h"
#include <ctime>

//------------------------------------------------------
// 모니터링 SS 프로토콜 (MonitorProtocol.h 에서 발췌)
//------------------------------------------------------
enum en_MONITOR_PACKET_TYPE
{
	en_PACKET_SS_MONITOR = 20000,
	en_PACKET_SS_MONITOR_LOGIN,
	en_PACKET_SS_MONITOR_DATA_UPDATE,
};

#define dfLAN_HEADERSIZE_LOCAL	(2)

CMonitorClient::CMonitorClient()
	: _serverNo(0)
{
}

CMonitorClient::~CMonitorClient()
{
	Disconnect();
}

bool CMonitorClient::ConnectToMonitor(const char* ip, int port, int serverNo)
{
	_serverNo = serverNo;

	if (!Connect(ip, port, 1, false))
	{
		printf("[MonitorClient] Connect failed\n");
		return false;
	}

	return true;
}

void CMonitorClient::SendLogin()
{
	CPacket* pPacket = CPacket::Alloc();
	// LAN 헤더 2바이트 공간 예약
	pPacket->_MsgheaderSize = dfLAN_HEADERSIZE_LOCAL;
	unsigned short dummy = 0;
	pPacket->PutData((char*)&dummy, dfLAN_HEADERSIZE_LOCAL);
	// 페이로드
	WORD type = en_PACKET_SS_MONITOR_LOGIN;
	*pPacket << type;
	*pPacket << (int)_serverNo;

	pPacket->AddRef();
	SendPacket(pPacket);
	pPacket->SubRef();

	printf("[MonitorClient] Login sent (ServerNo=%d)\n", _serverNo);
}

void CMonitorClient::SendMonitorData(BYTE dataType, int dataValue, int timeStamp)
{
	if (!IsConnected()) return;

	CPacket* pPacket = CPacket::Alloc();
	// LAN 헤더 2바이트 공간 예약
	pPacket->_MsgheaderSize = dfLAN_HEADERSIZE_LOCAL;
	unsigned short dummy = 0;
	pPacket->PutData((char*)&dummy, dfLAN_HEADERSIZE_LOCAL);
	// 페이로드
	WORD type = en_PACKET_SS_MONITOR_DATA_UPDATE;
	*pPacket << type;
	*pPacket << dataType;
	*pPacket << dataValue;
	*pPacket << timeStamp;

	pPacket->AddRef();
	SendPacket(pPacket);
	pPacket->SubRef();
}

//=============================================================
// CLanClient 콜백
//=============================================================

void CMonitorClient::OnEnterJoinServer()
{
	printf("[MonitorClient] Connected to monitoring server\n");
	SendLogin();
}

void CMonitorClient::OnLeaveServer()
{
	printf("[MonitorClient] Disconnected from monitoring server\n");
}

void CMonitorClient::OnRecv(CPacket* pPacket)
{
	// 모니터링 서버 → 내부 서버 방향 패킷은 현재 프로토콜에 정의 없음
	// 향후 확장을 위해 로그만 남김
	WORD type;
	*pPacket >> type;
	printf("[MonitorClient] Unexpected packet type: %d\n", type);
	pPacket->SubRef();
}

void CMonitorClient::OnError(int errorcode, const char* msg)
{
	printf("[MonitorClient Error] %d: %s\n", errorcode, msg);
}
