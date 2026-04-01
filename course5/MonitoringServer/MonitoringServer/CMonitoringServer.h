#pragma once
//------------------------------------------------------------
// CMonitoringServer - 모니터링 서버 컨텐츠 레이어
//
// 내부 서버(LAN) ↔ 모니터링 서버 ↔ 모니터링 툴(WAN)
//
// - CLanServerImpl: 내부 서버들의 접속을 받아 모니터 데이터 수신
// - CNetServerImpl: 모니터링 툴의 접속을 받아 데이터 전달
//------------------------------------------------------------

#include "CLanServer.h"
#include "CNetServer.h"
#include "SessionKey.h"
#include <unordered_map>
#include <vector>
#include <string>

#define dfLOGIN_KEY		"ajfw@!cv980dSZ[fje#@fdj123948djf"
#define dfLOGIN_KEY_LEN	(32)

#define dfMAX_SERVER_NO	(64)

class CMonitoringServer
{
public:
	CMonitoringServer();
	~CMonitoringServer();

	bool Start();
	void Stop();

private:

	//------------------------------------------------------------
	// LAN 서버 (내부 서버 접속용)
	//------------------------------------------------------------
	class CLanServerImpl : public CLanServer
	{
	public:
		CLanServerImpl(CMonitoringServer* pOwner) : _pOwner(pOwner) {}

	protected:
		bool OnConnectionRequest(std::string IP, int Port) override;
		void OnClientJoin(SOCKADDR_IN Client, SessionKey s) override;
		void OnClientLeave(SessionKey s) override;
		void OnRecv(SessionKey s, CPacket* pPacket) override;
		void OnError(int errorcode, const char* msg) override;

	private:
		CMonitoringServer* _pOwner;
	};

	//------------------------------------------------------------
	// NET 서버 (모니터링 툴 접속용)
	//------------------------------------------------------------
	class CNetServerImpl : public CNetServer
	{
	public:
		CNetServerImpl(CMonitoringServer* pOwner) : _pOwner(pOwner) {}

	protected:
		bool OnConnectionRequest(std::string IP, int Port) override;
		void OnClientJoin(SOCKADDR_IN Client, SessionKey s) override;
		void OnClientLeave(SessionKey s) override;
		void OnRecv(SessionKey s, CPacket* pPacket) override;
		void OnError(int errorcode, const char* msg) override;

	private:
		CMonitoringServer* _pOwner;
	};

	//------------------------------------------------------------
	// LAN 패킷 처리 (내부 서버 → 모니터링 서버)
	//------------------------------------------------------------
	void Handle_SS_MONITOR_LOGIN(SessionKey lanSession, CPacket* pPacket);
	void Handle_SS_MONITOR_DATA_UPDATE(SessionKey lanSession, CPacket* pPacket);

	//------------------------------------------------------------
	// NET 패킷 처리 (모니터링 툴 → 모니터링 서버)
	//------------------------------------------------------------
	void Handle_CS_MONITOR_TOOL_REQ_LOGIN(SessionKey netSession, CPacket* pPacket);

	//------------------------------------------------------------
	// 모니터링 데이터를 모든 인증된 NET 클라이언트에 전달
	//------------------------------------------------------------
	void BroadcastToMonitorClients(BYTE serverNo, BYTE dataType, int dataValue, int timeStamp);

	CLanServerImpl _lanServer;
	CNetServerImpl _netServer;

	//------------------------------------------------------------
	// 화면 갱신 스레드
	//------------------------------------------------------------
	static unsigned int __stdcall DisplayThread(LPVOID arg);
	HANDLE _hDisplayThread = NULL;
	volatile bool _bDisplayAlive = false;

	//------------------------------------------------------------
	// 서버 데이터 출력 스레드
	//------------------------------------------------------------
	static unsigned int __stdcall MonitorThread(LPVOID arg);
	HANDLE _hMonitorThread = NULL;
	bool _bMonitorAlive = false;

	//------------------------------------------------------------
	// 진단 카운터
	//------------------------------------------------------------
	volatile long _lanRecvCount = 0;
	volatile long _netBroadcastCount = 0;
	volatile long _netSendFailCount = 0;

	//------------------------------------------------------------
	// 서버별 수신 카운터 (serverNo 인덱스)
	//------------------------------------------------------------
	alignas(64) volatile long _serverRecvCount[dfMAX_SERVER_NO] = {};
	volatile bool _serverConnected[dfMAX_SERVER_NO] = {};

	//------------------------------------------------------------
	// LAN 세션 → ServerNo 매핑
	//------------------------------------------------------------
	SRWLOCK _lanSessionLock;
	std::unordered_map<unsigned long long, int> _lanSessionToServerNo;

	//------------------------------------------------------------
	// 인증된 NET 세션 목록
	//------------------------------------------------------------
	SRWLOCK _netSessionLock;
	std::vector<SessionKey> _authedNetSessions;
};
