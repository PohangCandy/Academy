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

//#ifndef __GODDAMNBUG_ONLINE_PROTOCOL__
//#define __GODDAMNBUG_ONLINE_PROTOCOL__



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
	en_PACKET_CS_CHAT_SERVER = 0,

	//------------------------------------------------------------
	// ä�ü��� �α��� ��û
	//
	//	{
	//		WORD	Type
	//
	//		INT64	AccountNo
	//		WCHAR	ID[20]				// null ����
	//		WCHAR	Nickname[20]		// null ����
	//		char	SessionKey[64];
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
	en_PACKET_CS_CHAT_RES_LOGIN,

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
	en_PACKET_CS_CHAT_RES_SECTOR_MOVE,

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
	en_PACKET_CS_CHAT_RES_MESSAGE,

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






	//------------------------------------------------------
	// Login Server
	//------------------------------------------------------
	en_PACKET_CS_LOGIN_SERVER = 100,

	//------------------------------------------------------------
	// �α��� ������ Ŭ���̾�Ʈ �α��� ��û
	//
	//	{
	//		WORD	Type
	//
	//		INT64	AccountNo
	//		char	SessionKey[64]
	//	}
	//
	//------------------------------------------------------------
	en_PACKET_CS_LOGIN_REQ_LOGIN,

	//------------------------------------------------------------
	// LoginServer -> Client : 로그인 응답
	//
	//	{
	//		WORD	Type
	//
	//		INT64	AccountNo
	//		BYTE	Status              // dfLOGIN_STATUS_*
	//
	//		WCHAR	ID[20]              // null 포함
	//		WCHAR	Nickname[20]        // null 포함
	//
	//		WCHAR	GameServerIP[16]    // 게임/채팅 서버 접속 정보
	//		USHORT	GameServerPort
	//		WCHAR	ChatServerIP[16]
	//		USHORT	ChatServerPort
	//
	//		char	SessionToken[64]    // [추가] 채팅/게임 서버에 제출할 토큰.
	//		                            // 현재는 DB 의 account.token 컬럼을 그대로
	//		                            // 전달하는 더미 구현. 다음 PR 에서 TokenServer
	//		                            // 가 발급한 일회용 토큰으로 교체 예정.
	//	}
	//
	//------------------------------------------------------------
	en_PACKET_CS_LOGIN_RES_LOGIN,








	//------------------------------------------------------
	// Game Server
	//------------------------------------------------------
	en_PACKET_CS_GAME_SERVER = 1000,





	////////////////////////////////////////////////////////
	//
	//   Server & Server Protocol  / LAN ����� �⺻���� ������ ���� ����.
	//
	////////////////////////////////////////////////////////
	en_PACKET_SS_LAN = 10000,
	//------------------------------------------------------
	// GameServer & LoginServer & ChatServer Protocol
	//------------------------------------------------------

	//------------------------------------------------------------
	// �ٸ� ������ �α��� ������ �α���.
	// �̴� ������ ������, �׳� �α��� ��.  
	//
	//	{
	//		WORD	Type
	//
	//		BYTE	ServerType			// dfSERVER_TYPE_GAME / dfSERVER_TYPE_CHAT
	//
	//		WCHAR	ServerName[32]		// �ش� ������ �̸�.  
	//	}
	//
	//------------------------------------------------------------
	en_PACKET_SS_LOGINSERVER_LOGIN,



	//------------------------------------------------------------
	// �α��μ������� ����.ä�� ������ ���ο� Ŭ���̾�Ʈ ������ �˸�.
	//
	//	{
	//		WORD	Type
	//
	//		INT64	AccountNo
	//		CHAR	SessionKey[64]
	//	}
	//
	//------------------------------------------------------------
	// en_PACKET_SS_NEW_CLIENT_LOGIN,	// �ű� �������� ����Ű ������Ŷ�� ��û,���䱸���� ���� 2017.01.05


	//------------------------------------------------------------
	// �α��μ������� ����.ä�� ������ ���ο� Ŭ���̾�Ʈ ������ �˸�.
	//
	// �������� Parameter �� ����Ű ������ ���� ������ Ȯ���� ���� � ��. �̴� ���� ������� �ٽ� �ް� ��.
	// ä�ü����� ���Ӽ����� Parameter �� ���� ó���� �ʿ� ������ �״�� Res �� ������� �մϴ�.
	//
	//	{
	//		WORD	Type
	//
	//		INT64	AccountNo
	//		CHAR	SessionKey[64]
	//		INT64	Parameter
	//	}
	//
	//------------------------------------------------------------
	en_PACKET_SS_REQ_NEW_CLIENT_LOGIN,

	//------------------------------------------------------------
	// ����.ä�� ������ ���ο� Ŭ���̾�Ʈ ������Ŷ ���Ű���� ������.
	// ���Ӽ�����, ä�ü����� ��Ŷ�� ������ ������, �α��μ����� Ÿ ������ ���� �� CHAT,GAME ������ �����ϹǷ� 
	// �̸� ����ؼ� �˾Ƽ� ���� �ϵ��� ��.
	//
	// �÷��̾��� ���� �α��� �Ϸ�� �� ��Ŷ�� Chat,Game ���ʿ��� �� �޾��� ������.
	//
	// ������ �� Parameter �� �̹� ����Ű ������ ���� ������ �� �ִ� Ư�� ��
	// ClientID �� ����, ���� ī������ ���� ��� ����.
	//
	// �α��μ����� ���Ӱ� �������� �ݺ��ϴ� ��� ������ ���������� ���� ������ ���� ��������
	// �����Ͽ� �ٸ� ����Ű�� ��� ���� ������ ����.
	//
	//	{
	//		WORD	Type
	//
	//		INT64	AccountNo
	//		INT64	Parameter
	//	}
	//
	//------------------------------------------------------------
	en_PACKET_SS_RES_NEW_CLIENT_LOGIN,




};



enum en_PACKET_CS_LOGIN_RES_LOGIN
{
	dfLOGIN_STATUS_NONE = -1,		// ����������
	dfLOGIN_STATUS_FAIL = 0,		// ���ǿ���
	dfLOGIN_STATUS_OK = 1,		// ����
	dfLOGIN_STATUS_GAME = 2,		// ������
	dfLOGIN_STATUS_ACCOUNT_MISS = 3,		// account ���̺��� AccountNo ����
	dfLOGIN_STATUS_SESSION_MISS = 4,		// Session ���̺��� AccountNo ����
	dfLOGIN_STATUS_STATUS_MISS = 5,		// Status ���̺��� AccountNo ����
	dfLOGIN_STATUS_NOSERVER = 6,		// �������� ������ ����.
};


enum en_PACKET_CS_GAME_RES_LOGIN
{
	dfGAME_LOGIN_FAIL = 0,		// ����Ű ���� �Ǵ� Account ���̺����� ����
	dfGAME_LOGIN_OK = 1,		// ����
	dfGAME_LOGIN_NOCHARACTER = 2,		// ���� / ĳ���� ���� > ĳ���� ����ȭ������ ��ȯ. 
	dfGAME_LOGIN_VERSION_MISS = 3,		// ����,Ŭ�� ���� �ٸ�
};



enum en_PACKET_SS_LOGINSERVER_LOGIN
{
	dfSERVER_TYPE_GAME = 1,
	dfSERVER_TYPE_CHAT = 2,
	dfSERVER_TYPE_MONITOR = 3,
};

enum en_PACKET_SS_HEARTBEAT
{
	dfTHREAD_TYPE_WORKER = 1,
	dfTHREAD_TYPE_DB = 2,
	dfTHREAD_TYPE_GAME = 3,
};

// en_PACKET_SS_MONITOR_LOGIN
enum en_PACKET_CS_MONITOR_TOOL_SERVER_CONTROL
{
	dfMONITOR_SERVER_TYPE_LOGIN = 1,
	dfMONITOR_SERVER_TYPE_GAME = 2,
	dfMONITOR_SERVER_TYPE_CHAT = 3,
	dfMONITOR_SERVER_TYPE_AGENT = 4,

	dfMONITOR_SERVER_CONTROL_SHUTDOWN = 1,		// ���� �������� (���Ӽ��� ����)
	dfMONITOR_SERVER_CONTROL_TERMINATE = 2,		// ���� ���μ��� ��������
	dfMONITOR_SERVER_CONTROL_RUN = 3,		// ���� ���μ��� ���� & ����
};


enum en_PACKET_SS_MONITOR_DATA_UPDATE
{
	dfMONITOR_DATA_TYPE_LOGIN_SESSION = 1,		// �α��μ��� ���� �� (���ؼ� ��)
	dfMONITOR_DATA_TYPE_LOGIN_AUTH_TPS = 2,		// �α��μ��� ���� ó�� �ʴ� Ƚ��
	dfMONITOR_DATA_TYPE_LOGIN_PACKET_POOL = 3,		// �α��μ��� ��ŶǮ ��뷮
	dfMONITOR_DATA_TYPE_LOGIN_SERVER_ON = 4,		// �������� ���� ����
	dfMONITOR_DATA_TYPE_LOGIN_LIVE_SERVER = 5,		// ���� ���̺� ���� ���� ��ȣ

	dfMONITOR_DATA_TYPE_GAME_SESSION = 6,		// ���Ӽ��� ���� �� (���ؼ� ��)
	dfMONITOR_DATA_TYPE_GAME_AUTH_PLAYER = 7,		// ���Ӽ��� AUTH MODE �÷��̾� ��
	dfMONITOR_DATA_TYPE_GAME_GAME_PLAYER = 8,		// ���Ӽ��� GAME MODE �÷��̾� ��
	dfMONITOR_DATA_TYPE_GAME_ACCEPT_TPS = 9,		// ���Ӽ��� Accept ó�� �ʴ� Ƚ��
	dfMONITOR_DATA_TYPE_GAME_PACKET_PROC_TPS = 10,		// ���Ӽ��� ��Ŷó�� �ʴ� Ƚ��
	dfMONITOR_DATA_TYPE_GAME_PACKET_SEND_TPS = 11,		// ���Ӽ��� ��Ŷ ������ �ʴ� �Ϸ� Ƚ��
	dfMONITOR_DATA_TYPE_GAME_DB_WRITE_TPS = 12,		// ���Ӽ��� DB ���� �޽��� �ʴ� ó�� Ƚ��
	dfMONITOR_DATA_TYPE_GAME_DB_WRITE_MSG = 13,		// ���Ӽ��� DB ���� �޽��� ���� ���� (���� ��)
	dfMONITOR_DATA_TYPE_GAME_AUTH_THREAD_FPS = 14,		// ���Ӽ��� AUTH ������ �ʴ� ������ �� (���� ��)
	dfMONITOR_DATA_TYPE_GAME_GAME_THREAD_FPS = 15,		// ���Ӽ��� GAME ������ �ʴ� ������ �� (���� ��)
	dfMONITOR_DATA_TYPE_GAME_PACKET_POOL = 16,		// ���Ӽ��� ��ŶǮ ��뷮

	dfMONITOR_DATA_TYPE_CHAT_SESSION = 17,		// ä�ü��� ���� �� (���ؼ� ��)
	dfMONITOR_DATA_TYPE_CHAT_PLAYER = 18,		// ä�ü��� �������� ����� �� (���� ������)
	dfMONITOR_DATA_TYPE_CHAT_UPDATE_TPS = 19,		// ä�ü��� UPDATE ������ �ʴ� �ʸ� Ƚ��
	dfMONITOR_DATA_TYPE_CHAT_PACKET_POOL = 20,		// ä�ü��� ��ŶǮ ��뷮
	dfMONITOR_DATA_TYPE_CHAT_UPDATEMSG_POOL = 21,		// ä�ü��� UPDATE MSG Ǯ ��뷮

	dfMONITOR_DATA_TYPE_AGENT_GAME_SERVER_RUN = 22,		// ������Ʈ GameServer ���� ���� ON / OFF
	dfMONITOR_DATA_TYPE_AGENT_CHAT_SERVER_RUN = 23,		// ������Ʈ ChatServer ���� ���� ON / OFF
	dfMONITOR_DATA_TYPE_AGENT_GAME_SERVER_CPU = 24,		// ������Ʈ GameServer CPU ����
	dfMONITOR_DATA_TYPE_AGENT_CHAT_SERVER_CPU = 25,		// ������Ʈ ChatServer CPU ����
	dfMONITOR_DATA_TYPE_AGENT_GAME_SERVER_MEM = 26,		// ������Ʈ GameServer �޸� ��� MByte
	dfMONITOR_DATA_TYPE_AGENT_CHAT_SERVER_MEM = 27,		// ������Ʈ ChatServer �޸� ��� MByte
	dfMONITOR_DATA_TYPE_AGENT_CPU_TOTAL = 28,		// ������Ʈ ������ǻ�� CPU ��ü ����
	dfMONITOR_DATA_TYPE_AGENT_NONPAGED_MEMORY = 29,		// ������Ʈ ������ǻ�� �������� �޸� MByte	// �ֱ� ����
	dfMONITOR_DATA_TYPE_AGENT_NETWORK_RECV = 30,		// ������Ʈ ������ǻ�� ��Ʈ��ũ ���ŷ� KByte	// �ֱ� ����
	dfMONITOR_DATA_TYPE_AGENT_NETWORK_SEND = 31,		// ������Ʈ ������ǻ�� ��Ʈ��ũ �۽ŷ� KByte	// �ֱ� ����
	dfMONITOR_DATA_TYPE_AGENT_ = 32,		// ������Ʈ ������ǻ�� 
	dfMONITOR_DATA_TYPE_AGENT_AVAILABLE_MEMORY = 33,		// ������Ʈ ������ǻ�� ��밡�� �޸�
};


enum en_PACKET_CS_MONITOR_TOOL_RES_LOGIN
{
	dfMONITOR_TOOL_LOGIN_OK = 1,		// �α��� ����
	dfMONITOR_TOOL_LOGIN_ERR_NOSERVER = 2,		// �����̸� ���� (��Ī�̽�)
	dfMONITOR_TOOL_LOGIN_ERR_SESSIONKEY = 3,		// �α��� ����Ű ����
};


//#endif