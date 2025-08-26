#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iostream>
#include <vector>
#include "Packet.h"
#include "Player.h"

#pragma comment(lib, "Winmm.lib")
#pragma comment(lib, "ws2_32")

#define SERVER_PORT 5000
#define MAX_CONNECT_COUNT 100

#define dfRANGE_MOVE_TOP	50
#define dfRANGE_MOVE_LEFT	10
#define dfRANGE_MOVE_RIGHT	630
#define dfRANGE_MOVE_BOTTOM	470

using namespace std;

vector<Player*> playerList;

vector<SOCKET> dummySocket;
WCHAR serverIp[INET_ADDRSTRLEN];

int Test1();
int Test2();
int Test3();
int Test4();

int main()
{
    srand(time(NULL));
    timeBeginPeriod(1);

    WSAData wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        return -1;

    wcout << "Enter IP: ";
    wcin >> serverIp;

    //Test1();
    //Test2();
    //Test3();
    Test4();

}

int Test1()
{
    for (int i = 0; i < MAX_CONNECT_COUNT; i++)
    {
        SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

        if (sock == INVALID_SOCKET)
        {
            WSACleanup();
            return -1;
        }


        SOCKADDR_IN server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(SERVER_PORT);
        InetPton(AF_INET, serverIp, &server_addr.sin_addr);

        int retconnect = connect(sock, (sockaddr*)&server_addr, sizeof(server_addr));

        if (retconnect != 0)
        {
            WSAGetLastError();
            closesocket(sock);
            WSACleanup();
            return -1;
        }
        PacketCreateMyCharacter packet(0, 0, 0, 0, 100);
        recv(sock, (char*)&packet, sizeof(packet), 0);

        Player* player = new Player(sock, 0, 0, packet._id, packet._direction,
            packet._x, packet._y, packet._hp);

        playerList.push_back(player);

        ULONG on = 1;
        ioctlsocket(sock, FIONBIO, &on);
    }

    while (1)
    {
        Sleep(10000);
    }
}

int Test2()
{
    for (int i = 0; i < MAX_CONNECT_COUNT; i++)
    {
        SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

        if (sock == INVALID_SOCKET)
        {
            WSACleanup();
            return -1;
        }


        SOCKADDR_IN server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(SERVER_PORT);
        InetPton(AF_INET, serverIp, &server_addr.sin_addr);

        int retconnect = connect(sock, (sockaddr*)&server_addr, sizeof(server_addr));

        if (retconnect != 0)
        {
            WSAGetLastError();
            closesocket(sock);
            WSACleanup();
            return -1;
        }
        PacketCreateMyCharacter packet(0, 0, 0, 0, 100);
        recv(sock, (char*)&packet, sizeof(packet), 0);

        Player* player = new Player(sock, 0, 0, packet._id, packet._direction,
            packet._x, packet._y, packet._hp);

        playerList.push_back(player);

        ULONG on = 1;
        ioctlsocket(sock, FIONBIO, &on);
    }


    while (1)
    {
        for (Player* player : playerList)
        {
            PacketAttack1CtoS packet;
            packet._direction = rand() % 2;
            packet._x = rand() % dfRANGE_MOVE_RIGHT + dfRANGE_MOVE_LEFT;
            packet._y = rand() % dfRANGE_MOVE_BOTTOM + dfRANGE_MOVE_TOP;
            send(player->_socket, (char*)&packet, sizeof(packet), 0);
        }
        Sleep(400);
    }
}

int Test3()
{
    for (int i = 0; i < 30; i++)
    {
        SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

        if (sock == INVALID_SOCKET)
        {
            WSACleanup();
            return -1;
        }


        SOCKADDR_IN server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(SERVER_PORT);
        InetPton(AF_INET, serverIp, &server_addr.sin_addr);

        int retconnect = connect(sock, (sockaddr*)&server_addr, sizeof(server_addr));

        if (retconnect != 0)
        {
            WSAGetLastError();
            closesocket(sock);
            WSACleanup();
            return -1;
        }
        PacketCreateMyCharacter packet(0, 0, 0, 0, 100);
        recv(sock, (char*)&packet, sizeof(packet), 0);

        Player* player = new Player(sock, 0, 0, packet._id, packet._direction,
            packet._x, packet._y, packet._hp);

        playerList.push_back(player);

        ULONG on = 1;
        ioctlsocket(sock, FIONBIO, &on);
    }


    while (1)
    {
        for (Player* player : playerList)
        {
            PacketAttack1CtoS packet;
            packet._direction = rand() % 2;
            packet._x = player->_x;
            packet._y = player->_y;
            send(player->_socket, (char*)&packet, sizeof(packet), 0);
        }
        Sleep(50);
    }
}

int Test4()
{
    for (int i = 0; i < MAX_CONNECT_COUNT; i++)
    {
        SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

        if (sock == INVALID_SOCKET)
        {
            WSACleanup();
            return -1;
        }


        SOCKADDR_IN server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(SERVER_PORT);
        InetPton(AF_INET, serverIp, &server_addr.sin_addr);

        int retconnect = connect(sock, (sockaddr*)&server_addr, sizeof(server_addr));

        if (retconnect != 0)
        {
            WSAGetLastError();
            closesocket(sock);
            WSACleanup();
            return -1;
        }
        PacketCreateMyCharacter packet(0, 0, 0, 0, 100);
        recv(sock, (char*)&packet, sizeof(packet), 0);

        Player* player = new Player(sock, 0, 0, packet._id, packet._direction,
            packet._x, packet._y, packet._hp);

        playerList.push_back(player);

        ULONG on = 1;
        ioctlsocket(sock, FIONBIO, &on);
    }


    while (1)
    {
        for (Player* player : playerList)
        {
            PacketAttack1CtoS packet;
            packet._direction = rand() % 2;
            packet._x = player->_x;
            packet._y = player->_y;
            send(player->_socket, (char*)&packet, sizeof(packet), 0);
        }
        Sleep(50);
    }
}