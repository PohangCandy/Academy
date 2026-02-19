//MySQL  C Cconnector Lib 경로 + 헤더
//
//C : \Program Files\MySQL\MySQL Server 8.0\lib
//C : \Program Files\MySQL\MySQL Server 8.0\include
//
//libmysql.lib / libmysql.dll    DLL 버전 lib
//
//mysqlclient.lib   static 빌드 lib
//
//상황에 맞게 사용
//
//--------------------------------------------------------------------------

//----------------------------------------
// bin에 있는 2개의 라이브러리도 포함시켜야 함.
// 아님 exe옆에 두던가
// libssl-3-x64
// libcrypto-3-x64
//----------------------------------------

//C Connector 에서  Mysql 8.0 연결시 아래 쿼리를 먼저 반영하여
//패스워드 방식을 예전 방식으로 변경 해야 합니다.
//
//ALTER USER 'root'@'localhost' IDENTIFIED WITH mysql_native_password BY 'password';

#include "C:\Program Files\MySQL\MySQL Server 8.0\include\mysql.h"
#include "C:\Program Files\MySQL\MySQL Server 8.0\include\errmsg.h"
#include <stdio.h>

int main()
{
	MYSQL conn;
	MYSQL* connection = NULL;
	MYSQL_RES* sql_result;
	MYSQL_ROW sql_row;
	int query_stat;

	// 초기화
	mysql_init(&conn);

	// DB 연결
	connection = mysql_real_connect(&conn, "127.0.0.1", "root", "vmfh1234!", "test", 3306, (char*)NULL, 0);
	if (connection == NULL)
	{
		// mysql_errno(&_MySQL);
		fprintf(stderr, "Mysql connection error : %s", mysql_error(&conn));
		return 1;
	}

	// Select 쿼리문
	const char* query = "SELECT * FROM account";	// From 다음 DB에 존재하는 테이블 명으로 수정하세요
	query_stat = mysql_query(connection, query);
	if (query_stat != 0)
	{
		printf("Mysql query error : %s", mysql_error(&conn));
		return 1;
	}

	// 결과출력
	sql_result = mysql_store_result(connection);		// 결과 전체를 미리 가져옴
	//	sql_result=mysql_use_result(connection);		// fetch_row 호출시 1개씩 가져옴

	while ((sql_row = mysql_fetch_row(sql_result)) != NULL)
	{
		printf("%2s %2s %s\n", sql_row[0], sql_row[1], sql_row[2]);
	}
	mysql_free_result(sql_result);

	// DB 연결닫기
	mysql_close(connection);

	//	int a = mysql_insert_id(connection);


	//	query_stat = mysql_set_server_option(connection, MYSQL_OPTION_MULTI_STATEMENTS_ON);
	//	mysql_next_result(connection);				// 멀티쿼리 사용시 다음 결과 얻기
	//	sql_result=mysql_store_result(connection);	// next_result 후 결과 얻음
	while (1)
	{

	}


	return 0;
}

