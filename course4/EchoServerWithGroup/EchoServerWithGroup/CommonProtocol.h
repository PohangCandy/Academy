//#ifndef __GODDAMNBUG_ONLINE_PROTOCOL__
//#define __GODDAMNBUG_ONLINE_PROTOCOL__

//#define CODEKEY (0x77)
#define dfPACKET_CODE		(0x77)
#define dfPACKET_KEY		(0x32)
#define dfPACKET_HEADERSIZE		(5)
#define dfSESSEIONMAPSIZE		(50000)

#pragma pack(push, 1)
struct PacketHeader
{
	unsigned char Code;//
	unsigned short Len;
	unsigned char RandKey;
	unsigned char CheckSum;
};
#pragma pack(pop)

enum class ReleaseResult : uint8_t
{
	Fail,
	Success,
	Released
};

enum en_PACKET_TYPE
{
	////////////////////////////////////////////////////////
	//
	//	Client & Server Protocol
	//
	////////////////////////////////////////////////////////

	//------------------------------------------------------
	// Chatting Server
	//------------------------------------------------------
	en_PACKET_CS_CHAT_SERVER			= 0,

	//------------------------------------------------------------
	// 채팅서버 로그인 요청
	//
	//	{
	//		WORD	Type
	//
	//		INT64	AccountNo
	//		WCHAR	ID[20]				// null 포함
	//		WCHAR	Nickname[20]		// null 포함
	//		char	SessionKey[64];		// 인증토큰
	//	}
	//
	//------------------------------------------------------------
	en_PACKET_CS_CHAT_REQ_LOGIN,

	//------------------------------------------------------------
	// 채팅서버 로그인 응답
	//
	//	{
	//		WORD	Type
	//
	//		BYTE	Status				// 0:실패	1:성공
	//		INT64	AccountNo
	//	}
	//
	//------------------------------------------------------------
en_PACKET_SC_CHAT_RES_LOGIN,

//------------------------------------------------------------
// 채팅서버 섹터 이동 요청
//
//	{
//		WORD	Type
//
//		INT64	AccountNo
//		WORD	SectorX
//		WORD	SectorY
//	}
//
//------------------------------------------------------------
en_PACKET_CS_CHAT_REQ_SECTOR_MOVE,

//------------------------------------------------------------
// 채팅서버 섹터 이동 결과
//
//	{
//		WORD	Type
//
//		INT64	AccountNo
//		WORD	SectorX
//		WORD	SectorY
//	}
//
//------------------------------------------------------------
en_PACKET_SC_CHAT_RES_SECTOR_MOVE,

//------------------------------------------------------------
// 채팅서버 채팅보내기 요청
//
//	{
//		WORD	Type
//
//		INT64	AccountNo
//		WORD	MessageLen
//		WCHAR	Message[MessageLen / 2]		// null 미포함
//	}
//
//------------------------------------------------------------
en_PACKET_CS_CHAT_REQ_MESSAGE,

//------------------------------------------------------------
// 채팅서버 채팅보내기 응답  (다른 클라가 보낸 채팅도 이걸로 받음)
//
//	{
//		WORD	Type
//
//		INT64	AccountNo
//		WCHAR	ID[20]						// null 포함
//		WCHAR	Nickname[20]				// null 포함
//		
//		WORD	MessageLen
//		WCHAR	Message[MessageLen / 2]		// null 미포함
//	}
//
//------------------------------------------------------------
en_PACKET_SC_CHAT_RES_MESSAGE,

//------------------------------------------------------------
// 하트비트
//
//	{
//		WORD		Type
//	}
//
//
// 클라이언트는 이를 30초마다 보내줌.
// 서버는 40초 이상동안 메시지 수신이 없는 클라이언트를 강제로 끊어줘야 함.
//------------------------------------------------------------	
en_PACKET_CS_CHAT_REQ_HEARTBEAT,


//-----------------------------------------------
// 타이머 스레드가 일정 주기로 신호를 주는 틱
//-----------------------------------------------
en_PACKET_SS_TIMER_TICK,

//-----------------------------------------------
//클라이언트 종료 메시지
//-----------------------------------------------
en_PACKET_SS_Session_Release,

//-----------------------------------------------
//캐릭터 생성 메시지
//-----------------------------------------------
en_PACKET_SS_Create_Character

};

//#endif