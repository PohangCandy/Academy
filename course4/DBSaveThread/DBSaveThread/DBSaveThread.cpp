#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "C:\Program Files\MySQL\MySQL Server 8.0\include\mysql.h"
#include <process.h>
#include <stdio.h>
#include "QueryProtocol.h"
#include "CPacketForMultiThread.h"
#include "CPacketRingBuffer.h"

CPacketRingBuffer gMessageQueue(1000);
HANDLE g_hEvent;
CRITICAL_SECTION gCR;

unsigned int __stdcall UpdateThread(LPVOID arg);
unsigned int __stdcall DBWriterThread(LPVOID arg);

int main()
{
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

        st_DBQUERY_HEADER head;
        int r = rand() % 5;

        switch (r)
        {
        case df_DBQUERY_MSG_LEVELUP:
        {
            head.Type = df_DBQUERY_MSG_LEVELUP;
            st_DBQUERY_MSG_LEVELUP levelup{ rand() % 5, rand() % 100 };

            *cpacket << head.Type;
            *cpacket << levelup.AccountNo;
            *cpacket << levelup.Level;
            break;
        }
        case df_DBQUERY_MSG_MONEY_ADD:
        {
            head.Type = df_DBQUERY_MSG_MONEY_ADD;
            st_DBQUERY_MSG_MONEY_ADD m{ rand() % 5, 10, 1 };

            *cpacket << head.Type;
            *cpacket << m.iAccountNo;
            *cpacket << m.iMoney;
            *cpacket << m.iWhy;
            break;
        }
        case df_DBQUERY_MSG_QUEST_COMPLETE:
        {
            head.Type = df_DBQUERY_MSG_QUEST_COMPLETE;
            st_DBQUERY_MSG_QUEST_COMPLETE q{ rand() % 5, rand() % 100 };

            *cpacket << head.Type;
            *cpacket << q.iAccountNo;
            *cpacket << q.iQuestID;
            break;
        }
        case df_DBQUERY_MSG_ITEM_BUY:
        {
            head.Type = df_DBQUERY_MSG_ITEM_BUY;
            st_DBQUERY_MSG_ITEM_BUY b{ rand() % 5, rand() % 100, 100, rand() % 10 };

            *cpacket << head.Type;
            *cpacket << b.iAccountNo;
            *cpacket << b.iItemID;
            *cpacket << b.iPrice;
            *cpacket << b.iSlot;
            break;
        }
        case df_DBQUERY_MSG_ITEM_TRADE:
        {
            head.Type = df_DBQUERY_MSG_ITEM_TRADE;
            st_DBQUERY_MSG_ITEM_TRADE t{ rand() % 5,rand() % 10,rand() % 5,rand() % 10,0,1 };

            *cpacket << head.Type;
            *cpacket << t.FromAccountNo;
            *cpacket << t.FromItemSlot;
            *cpacket << t.ToAccountNo;
            *cpacket << t.ToItemSlot;
            *cpacket << t.tradeMoney;
            *cpacket << t.Quantity;
            break;
        }
        }

        EnterCriticalSection(&gCR);
        gMessageQueue.Enqueue(cpacket);
        LeaveCriticalSection(&gCR);

        SetEvent(g_hEvent);

        Sleep(1); // 부하 조절
    }
}

unsigned int __stdcall DBWriterThread(LPVOID arg)
{
    MYSQL conn;
    mysql_init(&conn);

    MYSQL* connection = mysql_real_connect(&conn, "127.0.0.1", "root", "vmfh1234!", "game_a", 3306, NULL, 0);
    if (!connection)
    {
        printf("DB connect fail: %s\n", mysql_error(&conn));
        return 0;
    }

    char query[256];

    while (1)
    {
        WaitForSingleObject(g_hEvent, INFINITE);

        while (1)
        {
            CPacket* cpacket = nullptr;

            EnterCriticalSection(&gCR);
            bool ok = gMessageQueue.Dequeue(cpacket);
            LeaveCriticalSection(&gCR);

            if (!ok) break;

            st_DBQUERY_HEADER header;
            *cpacket >> header.Type;

            switch (header.Type)
            {
            case df_DBQUERY_MSG_LEVELUP:
            {
                st_DBQUERY_MSG_LEVELUP m;
                *cpacket >> m.AccountNo >> m.Level;

                sprintf_s(query,
                    "UPDATE account SET level=%d WHERE accountno=%lld",
                    m.Level, m.AccountNo);
                break;
            }
            case df_DBQUERY_MSG_MONEY_ADD:
            {
                st_DBQUERY_MSG_MONEY_ADD m;
                *cpacket >> m.iAccountNo >> m.iMoney >> m.iWhy;

                sprintf_s(query,
                    "UPDATE account SET money=money+%d WHERE accountno=%lld",
                    m.iMoney, m.iAccountNo);
                break;
            }
            case df_DBQUERY_MSG_QUEST_COMPLETE:
            {
                st_DBQUERY_MSG_QUEST_COMPLETE m;
                *cpacket >> m.iAccountNo >> m.iQuestID;

                sprintf_s(query,
                    "INSERT INTO quest(accountno,questid) VALUES(%lld,%d)",
                    m.iAccountNo, m.iQuestID);
                break;
            }
            case df_DBQUERY_MSG_ITEM_BUY:
            {
                st_DBQUERY_MSG_ITEM_BUY m;
                *cpacket >> m.iAccountNo >> m.iItemID >> m.iPrice >> m.iSlot;

                sprintf_s(query,
                    "INSERT INTO item(accountno,itemid,slot) VALUES(%lld,%d,%d)",
                    m.iAccountNo, m.iItemID, m.iSlot);
                break;
            }
            case df_DBQUERY_MSG_ITEM_TRADE:
            {
                st_DBQUERY_MSG_ITEM_TRADE m;
                *cpacket >> m.FromAccountNo >> m.FromItemSlot
                    >> m.ToAccountNo >> m.ToItemSlot
                    >> m.tradeMoney >> m.Quantity;

                sprintf_s(query,
                    "UPDATE item SET accountno=%lld WHERE accountno=%lld AND slot=%lld",
                    m.ToAccountNo, m.FromAccountNo, m.FromItemSlot);
                break;
            }
            default:
                continue;
            }

            if (mysql_query(connection, query) != 0)
            {
                printf("Query error: %s\n", mysql_error(connection));
            }

            cpacket->SubRef();
        }
    }

    mysql_close(connection);
    return 0;
}
