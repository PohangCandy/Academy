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
            rand() % 5, rand() % 10, rand() % 5, rand() % 10, 10, 1
        };

        *cpacket << (int)df_DBQUERY_MSG_ITEM_TRADE
            << m.FromAccountNo << m.FromItemSlot
            << m.ToAccountNo << m.ToItemSlot
            << m.tradeMoney << m.Quantity;

        EnterCriticalSection(&gCR);
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

    // [중요] MULTI_QUERY를 위해 마지막 인자에 CLIENT_MULTI_STATEMENTS 플래그 추가
    unsigned long flags = (gDBMode == MULTI_QUERY) ? CLIENT_MULTI_STATEMENTS : 0;

    MYSQL* connection = mysql_real_connect(&conn, "127.0.0.1", "root", "vmfh1234!", "game_a", 3306, NULL, flags);

    if (!connection)
    {
        printf("DB connect fail: %s\n", mysql_error(&conn));
        return 0;
    }

    char query[2048]; // 멀티쿼리 시 문자열이 길어지므로 넉넉하게 할당
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

            char sellerCol = 'a' + (m.FromItemSlot % 10);
            char buyerCol = 'a' + (m.ToItemSlot % 10);

            switch (gDBMode)
            {
            case SINGLE_QUERY:
                sprintf_s(query, "UPDATE account SET money=money-%d WHERE accountno=%lld", m.tradeMoney, m.ToAccountNo);
                mysql_query(connection, query);
                sprintf_s(query, "UPDATE account SET money=money+%d WHERE accountno=%lld", m.tradeMoney, m.FromAccountNo);
                mysql_query(connection, query);
                sprintf_s(query, "UPDATE account SET item_%c=item_%c+%d WHERE accountno=%lld", buyerCol, buyerCol, m.Quantity, m.ToAccountNo);
                mysql_query(connection, query);
                sprintf_s(query, "UPDATE account SET item_%c=item_%c-%d WHERE accountno=%lld", sellerCol, sellerCol, m.Quantity, m.FromAccountNo);
                mysql_query(connection, query);
                successCount += 4;
                break;

            case MULTI_QUERY:
                // 세미콜론(;)으로 쿼리 결합
                sprintf_s(query,
                    "UPDATE account SET money=money-%d WHERE accountno=%lld;"
                    "UPDATE account SET money=money+%d WHERE accountno=%lld;"
                    "UPDATE account SET item_%c=item_%c+%d WHERE accountno=%lld;"
                    "UPDATE account SET item_%c=item_%c-%d WHERE accountno=%lld;",
                    m.tradeMoney, m.ToAccountNo, m.tradeMoney, m.FromAccountNo,
                    buyerCol, buyerCol, m.Quantity, m.ToAccountNo,
                    sellerCol, sellerCol, m.Quantity, m.FromAccountNo);

                // C API에서는 mysql_query로 멀티쿼리 전송
                if (mysql_query(connection, query) == 0)
                {
                    // [필수] 모든 결과셋을 비워줘야 다음 쿼리 전송 가능
                    do {
                        MYSQL_RES* res = mysql_store_result(connection);
                        if (res) mysql_free_result(res);
                    } while (mysql_next_result(connection) == 0);
                    successCount += 4;
                }
                else {
                    printf("Multi Query Error: %s\n", mysql_error(connection));
                }
                break;

            case TRANSACTION:
                mysql_query(connection, "START TRANSACTION");
                sprintf_s(query, "UPDATE account SET money=money-%d WHERE accountno=%lld", m.tradeMoney, m.ToAccountNo);
                mysql_query(connection, query);
                sprintf_s(query, "UPDATE account SET money=money+%d WHERE accountno=%lld", m.tradeMoney, m.FromAccountNo);
                mysql_query(connection, query);
                sprintf_s(query, "UPDATE account SET item_%c=item_%c+%d WHERE accountno=%lld", buyerCol, buyerCol, m.Quantity, m.ToAccountNo);
                mysql_query(connection, query);
                sprintf_s(query, "UPDATE account SET item_%c=item_%c-%d WHERE accountno=%lld", sellerCol, sellerCol, m.Quantity, m.FromAccountNo);
                mysql_query(connection, query);
                mysql_query(connection, "COMMIT");
                successCount += 4;
                break;
            }

            cpacket->SubRef();

            ULONGLONG now = GetTickCount64();
            if (now - lastTick >= 1000)
            {
                printf("[Mode:%d] TPS: %d | Queue: %d\n", gDBMode, successCount, qsize);
                successCount = 0;
                lastTick = now;
            }
        }
    }
    mysql_close(connection);
    return 0;
}