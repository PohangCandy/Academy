#pragma once
#include "CNetServer.h"
#include <map>
#include <list>
#include <unordered_map>

enum EServerMode {
	None,
	QA
};

//채팅 서버에 로그인한 캐릭터를 저장해둔 맵
std::map<INT64, Character> mapCharacter;

//캐릭터 리스트를 담아둔 섹터 맵
std::unordered_map<INT64,Character*> umapCharcterSector[50][50];

class Character {
public:
	INT64	_AccountNo;
	WCHAR	_ID[20];	// null 포함
	WCHAR	_Nickname[20];	// null 포함
	char	_SessionKey[64];		// 인증토큰

	WORD	_SectorX;
	WORD	_SectorY;
};

class ChattingServer : public CNetServer
{
public:
	ChattingServer();
	~ChattingServer();
private:

	virtual bool OnConnectionRequest(std::string IP, int Port) override;
	//< accept 직후
	//return false; //시 클라이언트 거부.
	//return true; //시 접속 허용

	virtual void	OnClientJoin(SOCKADDR_IN clientaddr, SessionID s) override;/// 기타등등
	//< Accept 후 접속처리 완료 후 호출.
	//OnAccept(..)

	virtual void 	OnClientLeave(SessionID s) override;
	//< Release 후 호출
	//OnRelease(..)

	virtual void 	OnRecv(SessionID s, CPacket* pPacket)  override;
	//< 패킷 수신 완료 후
	//OnMessage(..)

	//	virtual void OnSend(g_SessionCounter, int sendsize) = 0;           < 패킷 송신 완료 후

	//	virtual void OnWorkerThreadBegin() = 0;                    < 워커스레드 GQCS 바로 하단에서 호출
	//	virtual void OnWorkerThreadEnd() = 0;                      < 워커스레드 1루프 종료 후

	virtual void OnError(int errorcode, char*) override;

	//컨텐츠 스레드 함수
	static unsigned int __stdcall ContentsThread(LPVOID arg);



	HANDLE contentHcp = {};

	EServerMode _serverMode = None;
};