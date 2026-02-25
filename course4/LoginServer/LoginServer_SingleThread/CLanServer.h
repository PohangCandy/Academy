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
#include "SessionKey.h"

class CPacket;
class SOCKETINFO;
class CLanServer;
class IOCPHandle;
struct PacketHeader;

class CLanServer
{

public:

	//-----------------------------------------
	// 디코딩
	//-----------------------------------------
	bool Decode(PacketHeader* pHeader, char* pc);

	//오픈 IP / 포트 / 워커스레드 수(생성수, 러닝수) / 나글옵션 / 최대접속자 수
	bool Start(); 
	void Stop();
	int GetSessionCount();

	bool Disconnect(SessionKey sessionId); // SESSION_ID
	bool SendPacket(SessionKey sessionId, CPacket* cp); // SESSION_ID

	//------------------------------------------
	// 세션 수신
	// 세션 수신 링버퍼 상태 확인 후 WSAbuf에 등록, 해당 소켓에 대해 WSARecv 호출
	//------------------------------------------
	bool WsaRecvSession(SOCKADDR_IN& clientaddr, SOCKETINFO* ptr);

	//------------------------------------------
	// 세션 송신
	// 세션 송신 링버퍼 상태 확인 후 WSAbuf에 등록, 해당 소켓에 대해 WSASend 호출
	//------------------------------------------
	bool SendPost(SOCKADDR_IN& clientaddr, SOCKETINFO* ptr);

	//------------------------------------------
	// 현제 세션의 SendFlag와 송신 링버퍼 안에 있는 잔류 데이터 크기로 송신을 시도할지 말지 정한다.
	//------------------------------------------
	bool CanSend(SOCKETINFO* ptr);

	//작업자 스레드 함수
	static unsigned int __stdcall WorkerThread(LPVOID arg);
	

	virtual bool OnConnectionRequest(std::string IP,int Port) = 0; 
	//< accept 직후
	//return false; //시 클라이언트 거부.
	//return true; //시 접속 허용

	virtual void	OnClientJoin(SOCKADDR_IN Client ,SessionKey s) = 0;/// 기타등등
	//< Accept 후 접속처리 완료 후 호출.
	//OnAccept(..)

	virtual void 	OnClientLeave(SessionKey s) = 0; 
	//< Release 후 호출
	//OnRelease(..)


	virtual void 	OnRecv(SessionKey s, CPacket* pPacket) = 0;
	//< 패킷 수신 완료 후
	//OnMessage(..)

	

	virtual void OnError(int errorcode, char*) = 0;

	int getAcceptTPS();
	int getRecvMessageTPS();
	int getSendMessageTPS();

	virtual void OnSend(SessionKey s, int sendsize) = 0;           //< 패킷 송신 완료 후

	//	virtual void OnWorkerThreadBegin() = 0;                    < 워커스레드 GQCS 바로 하단에서 호출
	//	virtual void OnWorkerThreadEnd() = 0;                      < 워커스레드 1루프 종료 후

	HANDLE _hWorkerThreadIOCP;

	int _sessionCount;

	//모니터링 항목 :
	int _acceptTPS;
	int _recvMessageTPS;
	int _sendMessageTPS;

};


