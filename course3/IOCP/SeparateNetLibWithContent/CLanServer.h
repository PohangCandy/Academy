//---------------------------------------------------------------------------------------------
// 프로젝트명: 
// 네트워크 라이브러리 모듈화
// 
// 목적:
// 하나의 네트워크 라이브러리를 클래스화 시킨 후, 
// 이후 다양한 서버 형태가 이를 상속받아 사용할 수 있도록 한다.
// 
// 방법 : 
// 네트워크 라이브러리의 공통적인 로직은 맴버로 구현하고
// 컨텐츠 부를 순수 가상함수로 구현해 각 컨텐츠의 성격에 맞게 이를 상속받아 사용할 수 있도록 만든다.
// 
// 결론 :
// 
// 
// 추후 예정 :
// 
// 
//---------------------------------------------------------------------------------------------

//CLanServer.h
#pragma once
#include "stdafx.h"

class CPacket;
class SOCKETINFO;
typedef long long SessionID;

class CLanServer
{
	//오픈 IP / 포트 / 워커스레드 수(생성수, 러닝수) / 나글옵션 / 최대접속자 수
	bool Start(); 
	void Stop();
	int GetSessionCount();

	bool Disconnect(SessionID sessionId); // SESSION_ID
	bool SendPacket(SessionID sessionId, CPacket* cp); // SESSION_ID

	void ReleaseSession(SOCKADDR_IN& clientaddr, SOCKETINFO*& ptr);
	bool WsaSendSession(SOCKADDR_IN& clientaddr, SOCKETINFO*& ptr);
	bool WsaRecvSession(SOCKADDR_IN& clientaddr, SOCKETINFO*& ptr);

	virtual bool OnConnectionRequest(std::string IP,int Port) = 0; 
	//< accept 직후
	//return false; //시 클라이언트 거부.
	//return true; //시 접속 허용

	virtual void	OnClientJoin(SOCKADDR_IN Client ,SessionID s) = 0;/// 기타등등
	//< Accept 후 접속처리 완료 후 호출.
	//OnAccept(..)

	virtual void 	OnClientLeave(SessionID s) = 0; 
	//< Release 후 호출
	//OnRelease(..)


	virtual void 	OnRecv(SessionID s, CPacket* pPacket) = 0;
	//< 패킷 수신 완료 후
	//OnMessage(..)

	//	virtual void OnSend(g_SessionCounter, int sendsize) = 0;           < 패킷 송신 완료 후

	//	virtual void OnWorkerThreadBegin() = 0;                    < 워커스레드 GQCS 바로 하단에서 호출
	//	virtual void OnWorkerThreadEnd() = 0;                      < 워커스레드 1루프 종료 후

	virtual void OnError(int errorcode, char*) = 0;

	int getAcceptTPS();
	int getRecvMessageTPS();
	int getSendMessageTPS();


	int _sessionCount;

	//모니터링 항목 :
	int _acceptTPS;
	int _recvMessageTPS;
	int _sendMessageTPS;

};


