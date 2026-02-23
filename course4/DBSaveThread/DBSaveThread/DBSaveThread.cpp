#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <process.h>
#include <stdio.h>
#include <time.h>
#include "C:\Program Files\MySQL\MySQL Server 8.0\include\mysql.h"
#include "QueryProtocol.h"
#include "CPacketForMultiThread.h"
#include "CPacketRingBuffer.h"

// 라이브러리 링크 (프로젝트 설정에서 추가해도 되지만 코드에 명시하면 편리합니다)
#pragma comment(lib, "libmysql.lib")

enum DB_MODE
{
    SINGLE_QUERY = 0,   // 4번 단일 쿼리
    MULTI_QUERY = 1,    // 한 번에 4쿼리 전송 (mysql_query 사용)
    TRANSACTION = 2     // 트랜잭션 사용
};

// 테스트하고 싶은 모드로 변경하세요
DB_MODE gDBMode = TRANSACTION;

CPacketRingBuffer gMessageQueue(10000);
HANDLE g_hEvent;
CRITICAL_SECTION gCR;

unsigned int __stdcall UpdateThread(LPVOID arg);
unsigned int __stdcall DBWriterThread(LPVOID arg);

int main()
{
    srand((unsigned int)time(NULL));
    InitializeCriticalSection(&gCR);
    g_hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    HANDLE hUpdate = (HANDLE)_beginthreadex(NULL, 0, UpdateThread, NULL, 0, NULL);
    HANDLE hDB = (HANDLE)_beginthreadex(NULL, 0, DBWriterThread, NULL, 0, NULL);

    WaitForSingleObject(hUpdate, INFINITE);
    WaitForSingleObject(hDB, INFINITE);

    DeleteCriticalSection(&gCR);
    CloseHandle(g_hEvent);
    return 0;
}

unsigned int __stdcall UpdateThread(LPVOID arg)
{
    while (1)
    {
        CPacket* cpacket = CPacket::Alloc();
        st_DBQUERY_MSG_ITEM_TRADE m{
            rand() % 5, rand() % 5, rand() % 5, rand() % 10, 10, 1
        };

        *cpacket << (int)df_DBQUERY_MSG_ITEM_TRADE
            << m.FromAccountNo << m.FromItemSlot
            << m.ToAccountNo << m.ToItemSlot
            << m.tradeMoney << m.Quantity;

        EnterCriticalSection(&gCR);
        cpacket->AddRef();
        bool ok = gMessageQueue.Enqueue(cpacket);
        LeaveCriticalSection(&gCR);

        if (!ok)
        {
            cpacket->SubRef();
            Sleep(1);
            continue;
        }
        SetEvent(g_hEvent);
    }
}

unsigned int __stdcall DBWriterThread(LPVOID arg)
{
    MYSQL conn;
    mysql_init(&conn);

    unsigned long flags = (gDBMode == MULTI_QUERY) ? CLIENT_MULTI_STATEMENTS : 0;
    MYSQL* connection = mysql_real_connect(&conn, "127.0.0.1", "root", "vmfh1234!", "game_a", 3306, NULL, flags);

    if (!connection)
    {
        printf("DB connect fail: %s\n", mysql_error(&conn));
        return 0;
    }

    char query[2048];
    int successCount = 0;
    ULONGLONG lastTick = GetTickCount64();

    while (1)
    {
        WaitForSingleObject(g_hEvent, INFINITE);

        while (1)
        {
            CPacket* cpacket = nullptr;
            EnterCriticalSection(&gCR);
            bool ok = gMessageQueue.Dequeue(cpacket);
            int qsize = gMessageQueue.GetUseSize();
            LeaveCriticalSection(&gCR);

            if (!ok) break;

            st_DBQUERY_MSG_ITEM_TRADE m;
            int type;
            *cpacket >> type;
            *cpacket >> m.FromAccountNo >> m.FromItemSlot >> m.ToAccountNo >> m.ToItemSlot >> m.tradeMoney >> m.Quantity;

            char sellerCol = 'a' + (m.FromItemSlot % 5);
            char buyerCol = 'a' + (m.ToItemSlot % 5);

            bool queryError = false;
            int totalAffectedRows = 0;

            // 람다 함수나 별도 함수로 빼면 깔끔하지만, 구조 유지를 위해 직접 작성합니다.
            auto ExecuteQuery = [&](const char* q) mutable -> bool {
                if (mysql_query(connection, q) != 0) {
                    printf("\n[Query Fail] %s\nError: %s\n", q, mysql_error(connection));
                    return false;
                }
                // 실제로 수정된 행이 있는지 확인 (WHERE절 일치 여부)
                totalAffectedRows += (int)mysql_affected_rows(connection);
                return true;
                };

            switch (gDBMode)
            {
            case SINGLE_QUERY:
                sprintf_s(query, "UPDATE account SET money=money-%d WHERE accountno=%lld", m.tradeMoney, m.ToAccountNo);
                if (!ExecuteQuery(query)) { queryError = true; break; }

                sprintf_s(query, "UPDATE account SET money=money+%d WHERE accountno=%lld", m.tradeMoney, m.FromAccountNo);
                if (!ExecuteQuery(query)) { queryError = true; break; }

                sprintf_s(query, "UPDATE account SET item_%c=item_%c+%d WHERE accountno=%lld", buyerCol, buyerCol, m.Quantity, m.ToAccountNo);
                if (!ExecuteQuery(query)) { queryError = true; break; }

                sprintf_s(query, "UPDATE account SET item_%c=item_%c-%d WHERE accountno=%lld", sellerCol, sellerCol, m.Quantity, m.FromAccountNo);
                if (!ExecuteQuery(query)) { queryError = true; break; }

                if (totalAffectedRows > 0) successCount++;
                break;

            case MULTI_QUERY:
                sprintf_s(query,
                    "UPDATE account SET money=money-%d WHERE accountno=%lld;"
                    "UPDATE account SET money=money+%d WHERE accountno=%lld;"
                    "UPDATE account SET item_%c=item_%c+%d WHERE accountno=%lld;"
                    "UPDATE account SET item_%c=item_%c-%d WHERE accountno=%lld;",
                    m.tradeMoney, m.ToAccountNo, m.tradeMoney, m.FromAccountNo,
                    buyerCol, buyerCol, m.Quantity, m.ToAccountNo,
                    sellerCol, sellerCol, m.Quantity, m.FromAccountNo);

                if (mysql_query(connection, query) == 0)
                {
                    int affectedInMulti = 0;
                    do {
                        affectedInMulti += (int)mysql_affected_rows(connection);
                        MYSQL_RES* res = mysql_store_result(connection);
                        if (res) mysql_free_result(res);
                    } while (mysql_next_result(connection) == 0);

                    if (affectedInMulti > 0) successCount++;
                }
                else {
                    printf("\n[Multi Query Fail] %s\n", mysql_error(connection));
                }
                break;

            case TRANSACTION:
                if (mysql_query(connection, "START TRANSACTION") == 0) {
                    bool stepFail = false;
                    sprintf_s(query, "UPDATE account SET money=money-%d WHERE accountno=%lld", m.tradeMoney, m.ToAccountNo);
                    if (!ExecuteQuery(query)) stepFail = true;

                    sprintf_s(query, "UPDATE account SET money=money+%d WHERE accountno=%lld", m.tradeMoney, m.FromAccountNo);
                    if (!ExecuteQuery(query)) stepFail = true;

                    sprintf_s(query, "UPDATE account SET item_%c=item_%c+%d WHERE accountno=%lld", buyerCol, buyerCol, m.Quantity, m.ToAccountNo);
                    if (!ExecuteQuery(query)) stepFail = true;

                    sprintf_s(query, "UPDATE account SET item_%c=item_%c-%d WHERE accountno=%lld", sellerCol, sellerCol, m.Quantity, m.FromAccountNo);
                    if (!ExecuteQuery(query)) stepFail = true;

                    if (!stepFail) {
                        mysql_query(connection, "COMMIT");
                        if (totalAffectedRows > 0) successCount++;
                    }
                    else {
                        mysql_query(connection, "ROLLBACK");
                        printf("\n[Transaction Rollbacked due to error]\n");
                    }
                }
                break;
            }

            cpacket->SubRef();

            ULONGLONG now = GetTickCount64();
            if (now - lastTick >= 1000)
            {
                printf("[Mode:%d] TPS: %d | Queue: %d\n", gDBMode, successCount, qsize);
                // 만약 TPS는 나오는데 데이터 변화가 없다면 totalAffectedRows가 0인 경우입니다.
                successCount = 0;
                lastTick = now;
            }
        }
    }
    mysql_close(connection);
    return 0;
}