#pragma once
//------------------------------------------------------------
// CMonitorClient - 모니터링 서버에 LAN 접속하여 데이터 전송
//
// CLanClient를 상속, SS_MONITOR 프로토콜 처리
// 채팅 서버 시작 시 Connect → Login → 주기적 DataUpdate
//------------------------------------------------------------

#include "CLanClient.h"

class CMonitorClient : public CLanClient
{
public:
	CMonitorClient();
	~CMonitorClient();

	//------------------------------------------------------------
	// 모니터링 서버 접속 및 로그인
	// serverNo: 이 서버의 고유 번호
	//------------------------------------------------------------
	bool ConnectToMonitor(const char* ip, int port, int serverNo);

	//------------------------------------------------------------
	// 모니터링 데이터 전송
	// dataType: en_PACKET_SS_MONITOR_DATA_UPDATE 에 정의된 값
	// dataValue: 해당 값
	// timeStamp: time() 캐스팅 값
	//------------------------------------------------------------
	void SendMonitorData(BYTE dataType, int dataValue, int timeStamp);

private:
	int _serverNo;

	// CLanClient 콜백
	void OnEnterJoinServer() override;
	void OnLeaveServer() override;
	void OnRecv(CPacket* pPacket) override;
	void OnError(int errorcode, const char* msg) override;

	void SendLogin();
};
