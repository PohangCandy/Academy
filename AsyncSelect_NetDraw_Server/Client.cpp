#define UNICODE
#define _UNICODE
#define _WINSOCK_DEPRECATED_NO_WARNINGS

// DrawingClient.cpp
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <tchar.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <vector> // 추가됨
#include "CRingBuffer.h"

#pragma comment(lib, "ws2_32.lib")

#define UM_NETWORK (WM_USER + 1)
#define SERVER_PORT 25000
#define PACKET_SIZE 18
#define HEADER_SIZE 2
#define BUFFER_SIZE 1800 + 1

struct stHEADER {
    unsigned short Len;
};

struct st_DRAW_PACKET {
    int iStartX;
    int iStartY;
    int iEndX;
    int iEndY;
};

bool g_bConnected = false;
SOCKET g_sock = INVALID_SOCKET;
CRingBuffer g_recvBuf(1800 + 1);  // 기본 버퍼 크기 지정
char g_tempBuf[BUFFER_SIZE];

HWND g_hWnd;
std::vector<st_DRAW_PACKET> g_drawPackets;
bool g_bMouseDown = false;  // 마우스 상태
int g_iPrevX = 0, g_iPrevY = 0;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void ProcessRead();
void ProcessWrite();
void SendDrawPacket(int sx, int sy, int ex, int ey);
void HandlePacket(char* data);
void ShowErrorMessage(const char* title, const char* msg);
void ForceCloseSocket();

void ShowErrorMessage(const char* title, const char* msg) {
    MessageBoxA(g_hWnd ? g_hWnd : NULL, msg, title, MB_ICONERROR | MB_OK);
}

void ForceCloseSocket() {
    if (g_sock != INVALID_SOCKET) {
        linger optLinger = { 1, 0 }; // RST 전송을 위한 linger 설정
        setsockopt(g_sock, SOL_SOCKET, SO_LINGER, (char*)&optLinger, sizeof(optLinger));
        closesocket(g_sock);
        g_sock = INVALID_SOCKET;
    }
    g_bConnected = false;
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        ShowErrorMessage("Startup Error", "WSAStartup failed");
        return 1;
    }

    WNDCLASSEXW wcex = {};
    wcex.cbSize = sizeof(WNDCLASSEXW);
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.lpszClassName = L"DrawingClientWindowClass";
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    if (!RegisterClassExW(&wcex)) {
        ShowErrorMessage("RegisterClassEx", "Window Class Registration Failed");
        return 1;
    }

    HWND hwnd = CreateWindowW(L"DrawingClientWindowClass", L"그리기 클라이언트", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        NULL, NULL, hInstance, NULL);

    if (!hwnd) {
        ShowErrorMessage("CreateWindow", "Window Creation Failed");
        return 1;
    }

    g_hWnd = hwnd;
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    g_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (g_sock == INVALID_SOCKET) {
        ShowErrorMessage("Socket", "Socket Creation Failed");
        return 1;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

    WSAAsyncSelect(g_sock, hwnd, UM_NETWORK, FD_CONNECT | FD_CLOSE | FD_READ | FD_WRITE);

    if (connect(g_sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        int err = WSAGetLastError();
        if (err != WSAEWOULDBLOCK && err != WSAEINPROGRESS) {
            ShowErrorMessage("Connect", "Connect Failed");
            closesocket(g_sock);
            return 1;
        }
    }

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    ForceCloseSocket();
    WSACleanup();
    return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case UM_NETWORK: {
        if (WSAGETSELECTERROR(lParam)) {
            char errLog[128];
            sprintf_s(errLog, "WSA Error: %d\n", WSAGETSELECTERROR(lParam));
            OutputDebugStringA(errLog);
            ShowErrorMessage("Socket Error", errLog);
            ForceCloseSocket();
            return 0;
        }

        switch (WSAGETSELECTEVENT(lParam)) {
        case FD_CONNECT:
            g_bConnected = true;
            OutputDebugStringA("Connected to server\n");
            break;
        case FD_CLOSE:
            g_bConnected = false;
            OutputDebugStringA("Connection closed\n");
            PostQuitMessage(0);
            break;
        case FD_READ:
            ProcessRead();
            break;
        case FD_WRITE:
            ProcessWrite();
            break;
        }
        break;
    }
    case WM_LBUTTONDOWN: {
        g_bMouseDown = true;
        g_iPrevX = LOWORD(lParam);
        g_iPrevY = HIWORD(lParam);
        break;
    }
    case WM_LBUTTONUP: {
        g_bMouseDown = false;
        break;
    }
    case WM_MOUSEMOVE: {
        if (!g_bConnected || !g_bMouseDown) break;
        int x = LOWORD(lParam);
        int y = HIWORD(lParam);
        SendDrawPacket(g_iPrevX, g_iPrevY, x, y);
        g_iPrevX = x;
        g_iPrevY = y;
        break;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        for (const auto& pkt : g_drawPackets) {
            MoveToEx(hdc, pkt.iStartX, pkt.iStartY, NULL);
            LineTo(hdc, pkt.iEndX, pkt.iEndY);
        }
        EndPaint(hWnd, &ps);
        break;
    }
    case WM_DESTROY:
        ForceCloseSocket(); // SO_LINGER 설정으로 RST 전송
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

void ProcessRead() {
    int ret = recv(g_sock, g_tempBuf, BUFFER_SIZE, 0);
    if (ret <= 0) {
        OutputDebugStringA("recv failed or connection closed\n");
        ShowErrorMessage("Recv Error", "recv failed or connection closed");
        ForceCloseSocket();
        return;
    }

    if (g_recvBuf.Enqueue(g_tempBuf, ret) != ret) {
        ShowErrorMessage("RingBuffer Error", "RingBuffer enqueue failed");
        return;
    }

    while (true) {
        stHEADER header;
        if (g_recvBuf.Peek((char*)&header, HEADER_SIZE) != HEADER_SIZE)
            break;

        unsigned short netLen = header.Len;
        if (g_recvBuf.GetUseSize() < HEADER_SIZE + netLen)
            break;

        g_recvBuf.MoveFront(HEADER_SIZE);

        char packetBuf[PACKET_SIZE]{};
        if (g_recvBuf.Dequeue(packetBuf, netLen) != netLen) {
            ShowErrorMessage("Dequeue Error", "Failed to dequeue full packet");
            break;
        }

        if (netLen != sizeof(st_DRAW_PACKET)) {
            char lenErr[128];
            sprintf_s(lenErr, "Invalid packet length: %d", netLen);
            ShowErrorMessage("Protocol Error", lenErr);
            ForceCloseSocket();
            return;
        }

        HandlePacket(packetBuf);
    }
}

void ProcessWrite() {
    // Optional if using SendQ
}

void SendDrawPacket(int sx, int sy, int ex, int ey) {
    char buffer[PACKET_SIZE]{};
    stHEADER header;
    header.Len = sizeof(st_DRAW_PACKET);

    memcpy(buffer, &header, HEADER_SIZE);

    st_DRAW_PACKET pkt{ sx, sy, ex, ey };
    memcpy(buffer + HEADER_SIZE, &pkt, sizeof(pkt));

    int ret = send(g_sock, buffer, PACKET_SIZE, 0);
    if (ret == SOCKET_ERROR) {
        char errMsg[128];
        sprintf_s(errMsg, "Send error: %d\n", WSAGetLastError());
        OutputDebugStringA(errMsg);
        ShowErrorMessage("Send Error", errMsg);
    }
}

void HandlePacket(char* data) {
    st_DRAW_PACKET* pkt = (st_DRAW_PACKET*)data;
    char log[128];
    sprintf_s(log, "Draw Packet: (%d,%d)->(%d,%d)\n", pkt->iStartX, pkt->iStartY, pkt->iEndX, pkt->iEndY);
    OutputDebugStringA(log);

    g_drawPackets.push_back(*pkt);
    InvalidateRect(g_hWnd, NULL, FALSE);
}
