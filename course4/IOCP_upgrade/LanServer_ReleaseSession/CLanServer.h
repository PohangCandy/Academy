//---------------------------------------------------------------------------------------------
// 프로젝트명: 
// 네트워크 라이브러리의 new, delete를 없애고 메모리 풀을 통해 할당받도록 만든다.
// 
// 목적:
// new, delete의 느림을 해결하자!
// 느린 이유
// 1. 캐시 미스 100% -> 매번 새롭게 할당하는 공간이므로 캐시에 없을 확률 100%
// 2. 힙 락 -> 힙은 프로세스의 공동 자원이므로 락이 걸린채 가져온다
// 3. 페이지 폴트 -> 최초 메모리 접근시 페이지 폴트 발생
// 
// 방법 : 
// new로 할당받는 모든 자원을 최대한 메모리 풀로 바꾼다.
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
class CLanServer;
class IOCPHandle;

struct ServerAndHandle
{
	CLanServer* thisptr;
	IOCPHandle* phandle;
};

class CLanServer
{

public:
	//오픈 IP / 포트 / 워커스레드 수(생성수, 러닝수) / 나글옵션 / 최대접속자 수
	bool Start(); 
	void Stop();
	int GetSessionCount();

	bool Disconnect(SessionID sessionId); // SESSION_ID
	bool SendPacket(SessionID sessionId, CPacket* cp); // SESSION_ID

	//------------------------------------------
	// 세션 수신
	// 세션 수신 링버퍼 상태 확인 후 WSAbuf에 등록, 해당 소켓에 대해 WSARecv 호출
	//------------------------------------------
	bool WsaRecvSession(SOCKADDR_IN& clientaddr, SOCKETINFO* ptr);

	//------------------------------------------
	// 세션 송신
	// 세션 송신 링버퍼 상태 확인 후 WSAbuf에 등록, 해당 소켓에 대해 WSASend 호출
	//------------------------------------------
	bool WsaSendSession(SOCKADDR_IN& clientaddr, SOCKETINFO* ptr);

	//-----------------------------------------
	// 세션 종료
	// IO가 끝난 세션에 대해 완전히 삭제
	//-----------------------------------------
	void ReleaseSession(SOCKADDR_IN& clientaddr, SOCKETINFO* ptr);

	//-----------------------------------------
	// 세션 IOCount를 줄이는 함수
	// -> interlock해도 되지만 곳곳에 ReleaseSession이 뿌려져 있는게 마음에 들지 않아서 묶음.
	// 원래는 무조건 ReleaseSession을 진행시킨다였지만 이젠 경우에 따라 ReleaseSession이 진행 되니 않고 그냥 decrease만 하는 경우도 존재
	// decreaseIO를 한 결과가 false면 ReleaseSession 성공으로 간주한다.
	//-----------------------------------------
	bool DecreaseSessionIO(SOCKADDR_IN& clientaddr, SOCKETINFO* ptr);

	//-----------------------------------------
	// 세션 IOCount를 증가시키는 함수
	// ReleaseFlag 비트를 제외한 나머지 비트에 대해서만 증가를 시킨다.
	//-----------------------------------------
	bool IncreaseSessionIO(SOCKETINFO* ptr);

	//작업자 스레드 함수
	static unsigned int __stdcall WorkerThread(LPVOID arg);
	

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


