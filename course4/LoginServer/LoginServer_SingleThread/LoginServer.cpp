#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "LoginServer.h"
#include "CPacketForMultiThread.h"
#include "CommonProtocol.h"
#include "MemoryPoolForLockFree.h"
#include "C:\Program Files\MySQL\MySQL Server 8.0\include\mysql.h"

#pragma comment(lib, "libmysql.lib")

#define BUFSIZE (1024 * 1024)
#define MSG_SIZE (8)

enum DB_MODE
{
	SINGLE_QUERY = 0,   // 4번 단일 쿼리
	MULTI_QUERY = 1,    // 한 번에 4쿼리 전송 (mysql_query 사용)
	TRANSACTION = 2     // 트랜잭션 사용
};

// 테스트하고 싶은 모드로 변경하세요
DB_MODE gDBMode = TRANSACTION;

LoginServer::LoginServer()
{
	//네트워크, 컨텐츠 스레드 입출력 완료 포트 생성
	hContentCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (hContentCompletionPort == NULL)
	{
		printf("[ChattingServer] 컨텐츠 스레드 IOCP 생성 실패");
		__debugbreak();
		return;
	}

	// ---------------------------------------------------------
		// [추가] 서버 시작 시 DB Status 초기화 (1회 수행)
		// ---------------------------------------------------------
	MYSQL initConn;
	mysql_init(&initConn);

	// 초기화 작업을 위한 일회성 연결
	if (mysql_real_connect(&initConn, "127.0.0.1", "root", "vmfh1234!", "accountdb", 3306, NULL, 0))
	{
		const char* initQuery = "UPDATE status SET status = 0";
		if (mysql_query(&initConn, initQuery) == 0)
		{
			//printf("[DB] 서버 시작: 모든 유저의 status를 0으로 초기화했습니다. (Affected: %lld)\n",(long long)mysql_affected_rows(&initConn));
		}
		else
		{
			printf("[DB Error] 초기화 쿼리 실패: %s\n", mysql_error(&initConn));
			__debugbreak(); // 초기화 실패 시 서버 구동 중단 고려
		}
		mysql_close(&initConn); // 사용 후 즉시 닫기
	}
	else
	{
		printf("[DB Error] 초기화용 DB 연결 실패: %s\n", mysql_error(&initConn));
		__debugbreak();
		return;
	}

	//컨텐츠 스레드 생성
	unsigned int uiThreadID;
	hContentThread = (HANDLE)_beginthreadex(
		NULL,          
		0,              
		ContentsThread,   
		this,    // 스레드에게 this포인터와 IOCP핸들 인자로 전달
		0,              
		&uiThreadID    
		);

		if (hContentThread == NULL)
		{
			printf("[ChattingServer] 컨텐츠 스레드 생성 실패");
			__debugbreak();
			return;
		}
}

LoginServer::~LoginServer()
{
	//컨텐츠 스레드 종료 유도
	PostQueuedCompletionStatus(
		hContentCompletionPort,
		0,
		0,
		nullptr
	);

	//컨텐츠 스레드 종료 대기
	WaitForSingleObject(hContentThread, INFINITE);
	CloseHandle(hContentThread);

	//컨텐츠 IOCP 핸들 반납
	CloseHandle(hContentCompletionPort);
}

//----------------------
// 특정 대역의 IP를 차단하고 싶을때, 들어온 IP와 일치하면 block
// 특정 대역의 IP만 허용시키고 싶을때, 들어온 IP와 일치하면 true
//----------------------
bool LoginServer::OnConnectionRequest(std::string IP, int Port)
{
    //if (_serverMode == QA)
    //{

    //}
    //else
    //{

    //}
    return true;
}

void LoginServer::OnClientJoin(SOCKADDR_IN clientaddr,SessionKey sessionkey)
{

}

void LoginServer::OnClientLeave(SessionKey sessionkey)
{

}

//컨텐츠 스레드 깨우기
void LoginServer::OnRecv(SessionKey sessionkey, CPacket* pPacket)
{
	pPacket->AddRef();

	if (!PostQueuedCompletionStatus(hContentCompletionPort, pPacket->GetDataSize(), (ULONG_PTR)sessionkey.GetSessionKey(), (LPWSAOVERLAPPED)pPacket))
	{
		printf("[OnRecv] 컨텐츠 IOCP에 PQCS실패!\n");
		__debugbreak();
	}
}

void LoginServer::OnSend(SessionKey s, int sendsize)
{
	Disconnect(s);
}

void LoginServer::OnError(int errorcode, char*)
{

}

//컨텐츠 스레드 함수
unsigned int __stdcall LoginServer::ContentsThread(LPVOID arg)
{
	int retval;

	LoginServer* pServer = (LoginServer*)arg;

	PacketHeader header;


	MYSQL conn;
	mysql_init(&conn);
	unsigned long flags = (gDBMode == MULTI_QUERY) ? CLIENT_MULTI_STATEMENTS : 0;
	MYSQL* connection = mysql_real_connect(&conn, "127.0.0.1", "root", "vmfh1234!", "accountdb", 3306, NULL, flags);

	if (!connection)
	{
		printf("DB connect fail: %s\n", mysql_error(&conn));
		return 0;
	}
	char query[2048];
	int successCount = 0;

	while (1) {
		//비동기 입출력 완료 기다리기
		DWORD cbTransferred;
		SOCKET client_sock;
		SessionKey sessionkey;

		CPacket* pPacket = nullptr;
		retval = GetQueuedCompletionStatus(pServer->hContentCompletionPort, &cbTransferred, (PULONG_PTR)&sessionkey, (LPOVERLAPPED*)&pPacket, INFINITE);


		//컨텐츠 스레드 종료
		if (cbTransferred == 0 && (&sessionkey) == nullptr && pPacket == nullptr)
		{
			break;
		}
		else if (retval == 0)
		{
			__debugbreak();
			printf("[Contents] 비동기 함수를 호출하지 않고는 나올 수 없는 경우\n");
		}

		WORD type;
		*pPacket >> type;

		int totalAffectedRows = 0;
		auto ExecuteQuery = [&](const char* q) mutable -> bool {
			if (mysql_query(connection, q) != 0) {
				printf("\n[Query Fail] %s\nError: %s\n", q, mysql_error(connection));
				return false;
			}
			// 실제로 수정된 행이 있는지 확인 (WHERE절 일치 여부)
			totalAffectedRows += (int)mysql_affected_rows(connection);
			return true;
			};

		switch (type)
		{
			//AccountNo로 조회 후 응답에 필요한 내용 던져주기
		case en_PACKET_CS_LOGIN_REQ_LOGIN:
		{
			INT64	AccountNo;
			char	SessionKey[64];
			*pPacket >> AccountNo;
			pPacket->GetData(SessionKey, sizeof(SessionKey));
			pPacket->SubRef();

			BYTE	Status = 0;		// 0 (세션오류) / 1 (성공) ...  하단 defines 사용

			WCHAR	ID[20];			// 사용자 ID		. null 포함
			WCHAR	Nickname[20];		// 사용자 닉네임	. null 포함

			WCHAR	GameServerIP[16] = L"127.0.0.1";	// 접속대상 게임,채팅 서버 정보
			USHORT	GameServerPort = 12345;
			WCHAR	ChatServerIP[16] = L"127.0.0.1";
			USHORT	ChatServerPort = 23456;

			// 1. 트랜잭션 시작
			mysql_query(connection, "START TRANSACTION");

			// 2. 유저 정보 조회 (accountdb 테이블)
			// 주의: DB의 char/varchar가 UTF-8이라면 MultiByteToWideChar 처리가 필요할 수 있습니다.
			sprintf_s(query, "SELECT userid, usernick FROM account WHERE AccountNo = %lld", AccountNo);

			if (mysql_query(connection, query) == 0) {
				MYSQL_RES* result = mysql_store_result(connection);
				if (result) {
					MYSQL_ROW row = mysql_fetch_row(result);
					if (row) {
						// DB 데이터를 WCHAR로 복사 (간단하게 전제)
						// 결과를 담을 변수와 성공 여부를 확인할 변수
						size_t convertedChars = 0;
						errno_t err;

						// 1. ID 변환 (row[0] -> ID)
						err = mbstowcs_s(&convertedChars, ID, 20, row[0], _TRUNCATE);
						if (err != 0) {
							// 변환 실패 처리 (예: 기본값 세팅)
							wcsncpy_s(ID, L"Unknown", 20);
						}

						// 2. Nickname 변환 (row[1] -> Nickname)
						err = mbstowcs_s(&convertedChars, Nickname, 20, row[1], _TRUNCATE);
						if (err != 0) {
							wcsncpy_s(Nickname, L"NoNick", 20);
						}

						// 3. Status 업데이트 (중복 로그인 방지 핵심)
						// status가 0인 경우에만 1로 업데이트 시도
						sprintf_s(query, "UPDATE status SET status = 1 WHERE AccountNo = %lld AND status = 0", AccountNo);

						if (mysql_query(connection, query) == 0) {
							if (mysql_affected_rows(connection) > 0) {
								// 성공: 유저가 존재했고, 상태도 0에서 1로 변경됨
								Status = 1;
								mysql_query(connection, "COMMIT");
							}
							else {
								// 실패: 이미 로그인 중(status=1)이거나 계정 없음
								Status = 0;
								__debugbreak();
								mysql_query(connection, "ROLLBACK");
							}
						}
						else {
							__debugbreak();
							mysql_query(connection, "ROLLBACK");
						}
					}
					else {
						// 계정 정보 없음
						__debugbreak();
						mysql_query(connection, "ROLLBACK");
					}
					mysql_free_result(result);
				}
			}
			else {
				__debugbreak();
				mysql_query(connection, "ROLLBACK");
			}



			// 3. 로그인 확인 메시지 전송
			CPacket* packetToSend = CPacket::Alloc();
			packetToSend->_MsgheaderSize = sizeof(PacketHeader);
			packetToSend->PutData((char*)&header, sizeof(PacketHeader));
			*packetToSend << (short)en_PACKET_CS_LOGIN_RES_LOGIN;
			*packetToSend << (INT64)AccountNo;
			*packetToSend << (BYTE)Status;
			packetToSend->PutData((char*)ID, sizeof(ID));
			packetToSend->PutData((char*)Nickname, sizeof(Nickname));
			packetToSend->PutData((char*)GameServerIP, sizeof(GameServerIP));
			*packetToSend<<(USHORT)GameServerPort;
			packetToSend->PutData((char*)ChatServerIP, sizeof(ChatServerIP));
			*packetToSend << (USHORT)ChatServerPort;

			packetToSend->AddRef();
			bool ret = pServer->SendPacket(sessionkey, packetToSend);
			packetToSend->SubRef();

			if (!ret)
			{
				printf("[Contents] SendPacket 실패, 세션 ID : %lld\n", sessionkey.GetSessionId());
				//__debugbreak();
			}

			break;
		}
		default:
			//말도 안되는 타입이 나왔다?
			//잘못된 패킷이 들어왔거나 인코딩 로직에 휴먼 에러
			__debugbreak();
			break;
		}
	}

	
	return 0;
}
