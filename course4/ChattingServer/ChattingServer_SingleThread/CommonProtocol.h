//#ifndef __GODDAMNBUG_ONLINE_PROTOCOL__
//#define __GODDAMNBUG_ONLINE_PROTOCOL__

//#define CODEKEY (0x77)
#define dfPACKET_CODE		(0x77)
#define dfPACKET_KEY		(0x32)
#define dfPACKET_HEADERSIZE	(5)

#pragma pack(push, 1)
struct PacketHeader
{
	unsigned char Code;//
	unsigned short Len;
	unsigned char RandKey;
	unsigned char CheckSum;
};
#pragma pack(pop)

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
	// ä�ü��� �α��� ��û
	//
	//	{
	//		WORD	Type
	//
	//		INT64	AccountNo
	//		WCHAR	ID[20]				// null ����
	//		WCHAR	Nickname[20]		// null ����
	//		char	SessionKey[64];		// ������ū
	//	}
	//
	//------------------------------------------------------------
	en_PACKET_CS_CHAT_REQ_LOGIN,

	//------------------------------------------------------------
	// ä�ü��� �α��� ����
	//
	//	{
	//		WORD	Type
	//
	//		BYTE	Status				// 0:����	1:����
	//		INT64	AccountNo
	//	}
	//
	//------------------------------------------------------------
	en_PACKET_SC_CHAT_RES_LOGIN,

	//------------------------------------------------------------
	// ä�ü��� ���� �̵� ��û
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
	// ä�ü��� ���� �̵� ���
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
	// ä�ü��� ä�ú����� ��û
	//
	//	{
	//		WORD	Type
	//
	//		INT64	AccountNo
	//		WORD	MessageLen
	//		WCHAR	Message[MessageLen / 2]		// null ������
	//	}
	//
	//------------------------------------------------------------
	en_PACKET_CS_CHAT_REQ_MESSAGE,

	//------------------------------------------------------------
	// ä�ü��� ä�ú����� ����  (�ٸ� Ŭ�� ���� ä�õ� �̰ɷ� ����)
	//
	//	{
	//		WORD	Type
	//
	//		INT64	AccountNo
	//		WCHAR	ID[20]						// null ����
	//		WCHAR	Nickname[20]				// null ����
	//		
	//		WORD	MessageLen
	//		WCHAR	Message[MessageLen / 2]		// null ������
	//	}
	//
	//------------------------------------------------------------
	en_PACKET_SC_CHAT_RES_MESSAGE,

	//------------------------------------------------------------
	// ��Ʈ��Ʈ
	//
	//	{
	//		WORD		Type
	//	}
	//
	//
	// Ŭ���̾�Ʈ�� �̸� 30�ʸ��� ������.
	// ������ 40�� �̻󵿾� �޽��� ������ ���� Ŭ���̾�Ʈ�� ������ ������� ��.
	//------------------------------------------------------------	
		en_PACKET_CS_CHAT_REQ_HEARTBEAT,


		//-----------------------------------------------
		// Ÿ�̸� �����尡 ���� �ֱ�� ��ȣ�� �ִ� ƽ
		//-----------------------------------------------
		en_PACKET_SS_TIMER_TICK
};

//--------------------------------------------------------------
// SS 내부 패킷 (세션 생성/삭제)
//--------------------------------------------------------------
enum en_PACKET_SS_TYPE
{
	en_PACKET_SS_Create_Character = 10000,
	en_PACKET_SS_Session_Release,
};

//--------------------------------------------------------------
// LAN 헤더 (내부 서버 간 통신, 암호화 없음)
//--------------------------------------------------------------
#define dfLAN_HEADERSIZE	(2)

#pragma pack(push, 1)
struct LanPacketHeader
{
	unsigned short Len;
};
#pragma pack(pop)

//--------------------------------------------------------------
// 세션 맵 / 전송 관련
//--------------------------------------------------------------
#define dfSEND_WSABUF_MAX	(128)

//--------------------------------------------------------------
// Release Flag / IOCount 비트 연산
//--------------------------------------------------------------
enum class ReleaseResult : uint8_t
{
	Fail,
	Success,
	Released
};

//#endif