// 모니터링 서버 프로토콜 정의
// CommonProtocol.h와 분리하여 모니터링 전용 패킷 정의

enum en_PACKET_TYPE
{
	//------------------------------------------------------
	// Monitor Server Protocol
	//------------------------------------------------------

	////////////////////////////////////////////////////////
	//   MonitorServer & MoniterTool Protocol
	////////////////////////////////////////////////////////

	//------------------------------------------------------
	// Server -> Monitor Protocol
	//------------------------------------------------------
	en_PACKET_SS_MONITOR					= 20000,

	//------------------------------------------------------------
	// 서버 -> 모니터링 서버 로그인
	//	{
	//		WORD	Type
	//		int		ServerNo
	//	}
	//------------------------------------------------------------
	en_PACKET_SS_MONITOR_LOGIN,

	//------------------------------------------------------------
	// 서버 -> 모니터링 서버 데이터 업데이트
	//	{
	//		WORD	Type
	//		BYTE	DataType
	//		int		DataValue
	//		int		TimeStamp
	//	}
	//------------------------------------------------------------
	en_PACKET_SS_MONITOR_DATA_UPDATE,


	en_PACKET_CS_MONITOR					= 25000,
	//------------------------------------------------------
	// Monitor -> Monitor Tool Protocol (Client <-> Server)
	//------------------------------------------------------

	//------------------------------------------------------------
	// 모니터링 클라이언트(툴) -> 모니터링 서버 로그인 요청
	//	{
	//		WORD	Type
	//		char	LoginSessionKey[32]
	//	}
	//------------------------------------------------------------
	en_PACKET_CS_MONITOR_TOOL_REQ_LOGIN,

	//------------------------------------------------------------
	// 모니터링 서버 -> 모니터링 클라이언트(툴) 로그인 응답
	//	{
	//		WORD	Type
	//		BYTE	Status
	//	}
	//------------------------------------------------------------
	en_PACKET_CS_MONITOR_TOOL_RES_LOGIN,

	//------------------------------------------------------------
	// 모니터링 서버 -> 모니터링 클라이언트(툴) 데이터 업데이트
	//	{
	//		WORD	Type
	//		BYTE	ServerNo
	//		BYTE	DataType
	//		int		DataValue
	//		int		TimeStamp
	//	}
	//------------------------------------------------------------
	en_PACKET_CS_MONITOR_TOOL_DATA_UPDATE,
};


enum en_PACKET_SS_MONITOR_DATA_UPDATE_TYPE
{
	dfMONITOR_DATA_TYPE_LOGIN_SERVER_RUN		= 1,
	dfMONITOR_DATA_TYPE_LOGIN_SERVER_CPU		= 2,
	dfMONITOR_DATA_TYPE_LOGIN_SERVER_MEM		= 3,
	dfMONITOR_DATA_TYPE_LOGIN_SESSION			= 4,
	dfMONITOR_DATA_TYPE_LOGIN_AUTH_TPS			= 5,
	dfMONITOR_DATA_TYPE_LOGIN_PACKET_POOL		= 6,

	dfMONITOR_DATA_TYPE_GAME_SERVER_RUN			= 10,
	dfMONITOR_DATA_TYPE_GAME_SERVER_CPU			= 11,
	dfMONITOR_DATA_TYPE_GAME_SERVER_MEM			= 12,
	dfMONITOR_DATA_TYPE_GAME_SESSION			= 13,
	dfMONITOR_DATA_TYPE_GAME_AUTH_PLAYER		= 14,
	dfMONITOR_DATA_TYPE_GAME_GAME_PLAYER		= 15,
	dfMONITOR_DATA_TYPE_GAME_ACCEPT_TPS			= 16,
	dfMONITOR_DATA_TYPE_GAME_PACKET_RECV_TPS	= 17,
	dfMONITOR_DATA_TYPE_GAME_PACKET_SEND_TPS	= 18,
	dfMONITOR_DATA_TYPE_GAME_DB_WRITE_TPS		= 19,
	dfMONITOR_DATA_TYPE_GAME_DB_WRITE_MSG		= 20,
	dfMONITOR_DATA_TYPE_GAME_AUTH_THREAD_FPS	= 21,
	dfMONITOR_DATA_TYPE_GAME_GAME_THREAD_FPS	= 22,
	dfMONITOR_DATA_TYPE_GAME_PACKET_POOL		= 23,

	dfMONITOR_DATA_TYPE_CHAT_SERVER_RUN			= 30,
	dfMONITOR_DATA_TYPE_CHAT_SERVER_CPU			= 31,
	dfMONITOR_DATA_TYPE_CHAT_SERVER_MEM			= 32,
	dfMONITOR_DATA_TYPE_CHAT_SESSION			= 33,
	dfMONITOR_DATA_TYPE_CHAT_PLAYER				= 34,
	dfMONITOR_DATA_TYPE_CHAT_UPDATE_TPS			= 35,
	dfMONITOR_DATA_TYPE_CHAT_PACKET_POOL		= 36,
	dfMONITOR_DATA_TYPE_CHAT_UPDATEMSG_POOL		= 37,

	dfMONITOR_DATA_TYPE_MONITOR_CPU_TOTAL		= 40,
	dfMONITOR_DATA_TYPE_MONITOR_NONPAGED_MEMORY	= 41,
	dfMONITOR_DATA_TYPE_MONITOR_NETWORK_RECV	= 42,
	dfMONITOR_DATA_TYPE_MONITOR_NETWORK_SEND	= 43,
	dfMONITOR_DATA_TYPE_MONITOR_AVAILABLE_MEMORY= 44,
};


enum en_MONITOR_TOOL_LOGIN_RESULT
{
	dfMONITOR_TOOL_LOGIN_OK					= 1,
	dfMONITOR_TOOL_LOGIN_ERR_NOSERVER		= 2,
	dfMONITOR_TOOL_LOGIN_ERR_SESSIONKEY		= 3,
};
