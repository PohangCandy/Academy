#define UNICODE
#define _UNICODE
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <tchar.h>
#include <stdio.h>
#include <vector>
#include "CRingBuffer.h"

#pragma comment(lib, "ws2_32.lib")

#define UM_NETWORK (WM_USER + 1)
#define SERVER_PORT 25000
#define HEADER_SIZE 2
#define PACKET_SIZE 18
#define BUFFER_SIZE 8192

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
CRingBuffer g_recvBuf(8192);
char g_tempBuf[BUFFER_SIZE];

HWND g_hWnd;
std::vector<st_DRAW_PACKET> g_drawPackets;

bool g_bMouseDown = false;
int g_oldX = 0, g_oldY = 0;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void ProcessRead();
void SendDrawPacket(int sx, int sy, int ex, int ey);
void HandlePacket(char* data);
void ShowErrorMessage(const char* title, const char* msg);

void ShowErrorMessage(const char* title, const char* msg) {
    MessageBoxA(g_hWnd ? g_hWnd : NULL, msg, title, MB_ICONERROR | MB_OK);
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

    closesocket(g_sock);
    WSACleanup();
    return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case UM_NETWORK: {
        if (WSAGETSELECTERROR(lParam)) {
            char errLog[128];
            sprintf_s(errLog, "WSA Error: %d\n", WSAGETSELECTERROR(lParam));
            ShowErrorMessage("Socket Error", errLog);
            closesocket(g_sock);
            g_bConnected = false;
            return 0;
        }

        switch (WSAGETSELECTEVENT(lParam)) {
        case FD_CONNECT:
            g_bConnected = true;
            break;
        case FD_CLOSE:
            g_bConnected = false;
            PostQuitMessage(0);
            break;
        case FD_READ:
            ProcessRead();
            break;
        case FD_WRITE:
            // 필요하면 구현
            break;
        }
        break;
    }
    case WM_LBUTTONDOWN:
        if (!g_bConnected) break;
        g_bMouseDown = true;
        g_oldX = LOWORD(lParam);
        g_oldY = HIWORD(lParam);
        break;

    case WM_MOUSEMOVE:
        if (g_bConnected && g_bMouseDown) {
            int curX = LOWORD(lParam);
            int curY = HIWORD(lParam);
            SendDrawPacket(g_oldX, g_oldY, curX, curY);
            g_oldX = curX;
            g_oldY = curY;
        }
        break;

    case WM_LBUTTONUP:
        g_bMouseDown = false;
        break;

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
        ShowErrorMessage("Recv Error", "recv failed or connection closed");
        closesocket(g_sock);
        g_bConnected = false;
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

        unsigned short len = header.Len; // 그대로 사용

        if (g_recvBuf.GetUseSize() < HEADER_SIZE + len)
            break;

        g_recvBuf.MoveFront(HEADER_SIZE);

        char packetBuf[PACKET_SIZE]{};
        if (g_recvBuf.Dequeue(packetBuf, len) != len) {
            ShowErrorMessage("Dequeue Error", "Failed to dequeue full packet");
            break;
        }

        if (len != sizeof(st_DRAW_PACKET)) {
            char lenErr[128];
            sprintf_s(lenErr, "Invalid packet length: %d", len);
            ShowErrorMessage("Protocol Error", lenErr);
            closesocket(g_sock);
            g_bConnected = false;
            return;
        }

        HandlePacket(packetBuf);
    }
}

void SendDrawPacket(int sx, int sy, int ex, int ey) {
    char buffer[PACKET_SIZE]{};
    stHEADER header;
    header.Len = sizeof(st_DRAW_PACKET); // 그대로 넣음

    memcpy(buffer, &header, HEADER_SIZE);

    st_DRAW_PACKET pkt{ sx, sy, ex, ey };
    memcpy(buffer + HEADER_SIZE, &pkt, sizeof(pkt));

    int ret = send(g_sock, buffer, PACKET_SIZE, 0);
    if (ret == SOCKET_ERROR) {
        char errMsg[128];
        sprintf_s(errMsg, "Send error: %d\n", WSAGetLastError());
        ShowErrorMessage("Send Error", errMsg);
    }
}

void HandlePacket(char* data) {
    st_DRAW_PACKET* pkt = (st_DRAW_PACKET*)data;
    g_drawPackets.push_back(*pkt);
    InvalidateRect(g_hWnd, NULL, FALSE);
}
