#pragma once

//=============================================================
// 모니터링 서버 공통 프로토콜 정의
//=============================================================

//--------------------------------------------------------------
// LAN 헤더 (내부 서버 간 통신, 암호화 없음)
//   [WORD Len] [payload...]
//--------------------------------------------------------------
#define dfLAN_HEADERSIZE	(2)

#pragma pack(push, 1)
struct LanPacketHeader
{
	unsigned short Len;
};
#pragma pack(pop)

//--------------------------------------------------------------
// NET 헤더 (외부 클라이언트 통신, 암호화 있음)
//   [BYTE Code] [WORD Len] [BYTE RandKey] [BYTE CheckSum] [encrypted payload...]
//--------------------------------------------------------------
#define dfNET_HEADERSIZE	(5)
#define dfNET_PACKET_CODE	(109)
#define dfNET_PACKET_KEY	(30)

#pragma pack(push, 1)
struct PacketHeader
{
	unsigned char Code;
	unsigned short Len;
	unsigned char RandKey;
	unsigned char CheckSum;
};
#pragma pack(pop)

//--------------------------------------------------------------
// 세션 맵 용량
//--------------------------------------------------------------
#define dfLAN_SESSION_MAX	(100)	// 내부 서버 최대 접속 수
#define dfNET_SESSION_MAX	(200)	// 모니터링 클라이언트 최대 접속 수

//--------------------------------------------------------------
// 버퍼 크기
//--------------------------------------------------------------
#define dfSEND_WSABUF_MAX	(128)

//--------------------------------------------------------------
// 서버 포트
//--------------------------------------------------------------
#define dfLAN_SERVER_PORT	(20000)	// 내부 서버용 LAN 포트
#define dfNET_SERVER_PORT	(10001)	// 모니터링 클라이언트용 WAN 포트

//--------------------------------------------------------------
// Release Flag / IOCount 비트 연산
//--------------------------------------------------------------
enum class ReleaseResult : uint8_t
{
	Fail,
	Success,
	Released
};
