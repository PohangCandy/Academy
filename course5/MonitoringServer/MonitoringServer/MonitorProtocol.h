#pragma once

//=============================================================
// 모니터링 서버 프로토콜 정의
//=============================================================

enum en_PACKET_TYPE
{
	//------------------------------------------------------
	// SS (Server -> Server) 모니터 프로토콜 (LAN)
	//------------------------------------------------------
	en_PACKET_SS_MONITOR = 20000,

	// 내부 서버 -> 모니터링 서버 로그인
	// { WORD Type, int ServerNo }
	en_PACKET_SS_MONITOR_LOGIN,

	// 내부 서버 -> 모니터링 서버 데이터 갱신
	// { WORD Type, BYTE DataType, int DataValue, int TimeStamp }
	en_PACKET_SS_MONITOR_DATA_UPDATE,

	//------------------------------------------------------
	// CS (Client <-> Server) 모니터 프로토콜 (WAN)
	//------------------------------------------------------
	en_PACKET_CS_MONITOR = 25000,

	// 모니터링 툴 -> 서버 로그인 요청
	// { WORD Type, char LoginSessionKey[32] }
	en_PACKET_CS_MONITOR_TOOL_REQ_LOGIN,

	// 서버 -> 모니터링 툴 로그인 응답
	// { WORD Type, BYTE Status }
	en_PACKET_CS_MONITOR_TOOL_RES_LOGIN,

	// 서버 -> 모니터링 툴 데이터 갱신
	// { WORD Type, BYTE ServerNo, BYTE DataType, int DataValue, int TimeStamp }
	en_PACKET_CS_MONITOR_TOOL_DATA_UPDATE,
};

//------------------------------------------------------
// 모니터링 데이터 타입 (SS_MONITOR_DATA_UPDATE 의 DataType)
//------------------------------------------------------
enum en_PACKET_SS_MONITOR_DATA_UPDATE
{
	dfMONITOR_DATA_TYPE_LOGIN_SERVER_RUN		= 1,	// 로그인서버 실행여부 ON / OFF
	dfMONITOR_DATA_TYPE_LOGIN_SERVER_CPU		= 2,	// 로그인서버 CPU 사용률
	dfMONITOR_DATA_TYPE_LOGIN_SERVER_MEM		= 3,	// 로그인서버 메모리 사용 MByte
	dfMONITOR_DATA_TYPE_LOGIN_SESSION			= 4,	// 로그인서버 세션 수
	dfMONITOR_DATA_TYPE_LOGIN_AUTH_TPS			= 5,	// 로그인서버 인증 처리 초당 횟수
	dfMONITOR_DATA_TYPE_LOGIN_PACKET_POOL		= 6,	// 로그인서버 패킷풀 사용량

	dfMONITOR_DATA_TYPE_GAME_SERVER_RUN			= 10,	// GameServer 동작 여부 ON / OFF
	dfMONITOR_DATA_TYPE_GAME_SERVER_CPU			= 11,	// GameServer CPU 사용률
	dfMONITOR_DATA_TYPE_GAME_SERVER_MEM			= 12,	// GameServer 메모리 사용 MByte
	dfMONITOR_DATA_TYPE_GAME_SESSION			= 13,	// 게임서버 세션 수
	dfMONITOR_DATA_TYPE_GAME_AUTH_PLAYER		= 14,	// 게임서버 AUTH MODE 플레이어 수
	dfMONITOR_DATA_TYPE_GAME_GAME_PLAYER		= 15,	// 게임서버 GAME MODE 플레이어 수
	dfMONITOR_DATA_TYPE_GAME_ACCEPT_TPS			= 16,	// 게임서버 Accept 초당 횟수
	dfMONITOR_DATA_TYPE_GAME_PACKET_RECV_TPS	= 17,	// 게임서버 패킷수신 초당 횟수
	dfMONITOR_DATA_TYPE_GAME_PACKET_SEND_TPS	= 18,	// 게임서버 패킷송신 초당 횟수
	dfMONITOR_DATA_TYPE_GAME_DB_WRITE_TPS		= 19,	// 게임서버 DB 저장 초당 횟수
	dfMONITOR_DATA_TYPE_GAME_DB_WRITE_MSG		= 20,	// 게임서버 DB 저장 메시지 큐 개수
	dfMONITOR_DATA_TYPE_GAME_AUTH_THREAD_FPS	= 21,	// 게임서버 AUTH 스레드 초당 프레임 수
	dfMONITOR_DATA_TYPE_GAME_GAME_THREAD_FPS	= 22,	// 게임서버 GAME 스레드 초당 프레임 수
	dfMONITOR_DATA_TYPE_GAME_PACKET_POOL		= 23,	// 게임서버 패킷풀 사용량

	dfMONITOR_DATA_TYPE_CHAT_SERVER_RUN			= 30,	// 채팅서버 동작 여부 ON / OFF
	dfMONITOR_DATA_TYPE_CHAT_SERVER_CPU			= 31,	// 채팅서버 CPU 사용률
	dfMONITOR_DATA_TYPE_CHAT_SERVER_MEM			= 32,	// 채팅서버 메모리 사용 MByte
	dfMONITOR_DATA_TYPE_CHAT_SESSION			= 33,	// 채팅서버 세션 수
	dfMONITOR_DATA_TYPE_CHAT_PLAYER				= 34,	// 채팅서버 인증성공 플레이어 수
	dfMONITOR_DATA_TYPE_CHAT_UPDATE_TPS			= 35,	// 채팅서버 UPDATE 초당 횟수
	dfMONITOR_DATA_TYPE_CHAT_PACKET_POOL		= 36,	// 채팅서버 패킷풀 사용량
	dfMONITOR_DATA_TYPE_CHAT_UPDATEMSG_POOL		= 37,	// 채팅서버 UPDATE MSG 풀 사용량

	dfMONITOR_DATA_TYPE_MONITOR_CPU_TOTAL		= 40,	// 서버컴퓨터 CPU 전체 사용률
	dfMONITOR_DATA_TYPE_MONITOR_NONPAGED_MEMORY	= 41,	// 서버컴퓨터 논페이지 메모리 MByte
	dfMONITOR_DATA_TYPE_MONITOR_NETWORK_RECV	= 42,	// 서버컴퓨터 네트워크 수신량 KByte
	dfMONITOR_DATA_TYPE_MONITOR_NETWORK_SEND	= 43,	// 서버컴퓨터 네트워크 송신량 KByte
	dfMONITOR_DATA_TYPE_MONITOR_AVAILABLE_MEMORY = 44,	// 서버컴퓨터 사용가능 메모리
};

//------------------------------------------------------
// 모니터링 툴 로그인 결과
//------------------------------------------------------
enum en_PACKET_CS_MONITOR_TOOL_RES_LOGIN
{
	dfMONITOR_TOOL_LOGIN_OK					= 1,	// 로그인 성공
	dfMONITOR_TOOL_LOGIN_ERR_NOSERVER		= 2,	// 서버이름 없음
	dfMONITOR_TOOL_LOGIN_ERR_SESSIONKEY		= 3,	// 로그인 세션키 오류
};
