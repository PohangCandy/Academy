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
		return false;
	}

	return true;
}

void CMonitorClient::SendLogin()
{
	CPacket* pPacket = CPacket::Alloc();
	pPacket->_MsgheaderSize = dfLAN_HEADERSIZE_LOCAL;
	unsigned short dummy = 0;
	pPacket->PutData((char*)&dummy, dfLAN_HEADERSIZE_LOCAL);
	WORD type = en_PACKET_SS_MONITOR_LOGIN;
	*pPacket << type;
	*pPacket << (int)_serverNo;

	pPacket->AddRef();
	SendPacket(pPacket);
	pPacket->SubRef();
}

void CMonitorClient::SendMonitorData(BYTE dataType, int dataValue, int timeStamp)
{
	if (!IsConnected()) return;

	CPacket* pPacket = CPacket::Alloc();
	pPacket->_MsgheaderSize = dfLAN_HEADERSIZE_LOCAL;
	unsigned short dummy = 0;
	pPacket->PutData((char*)&dummy, dfLAN_HEADERSIZE_LOCAL);
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
	SendLogin();
}

void CMonitorClient::OnLeaveServer()
{
}

void CMonitorClient::OnRecv(CPacket* pPacket)
{
	WORD type;
	*pPacket >> type;
	pPacket->SubRef();
}

void CMonitorClient::OnError(int errorcode, const char* msg)
{
}
