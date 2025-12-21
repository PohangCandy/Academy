//---------------------------------------------------------------------------------------------
// 프로젝트명: Select 모델을 이용한 MMOTCPFighter Server 만들기
// 
// 목적: Select 모델을 사용해 1만명 이상 플레이어가 접속 가능한 TCP 서버 만들기
// 
// 
// 방법 : 걍 Select 서버 모델에 1만명 이상이 접속하고 끊어질 수 있게 해주면 되는거 아님?
// 여기에 섹터를 추가해서 부하를 줄이면 될 듯
// 
// 
// 결론 :
// 
//---------------------------------------------------------------------------------------------
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "winmm.lib")

#include "stdafx.h"

#include <chrono>
#include "Session.h"
#include "Protocol.h"
#include "CRingbuffer.h"
#include <iostream>
#include <vector>
#include "cSessionMap.h"
#include "CPacket.h"
using namespace std;



// 전역
SOCKET g_ListenSocket = INVALID_SOCKET;
bool g_bShutdown = false;
uint32_t g_nextSessionID = 1;
cSessionMap* g_sessionMap = cSessionMap::GetSessionMap();

//vector <SOCKETINFO* > g_Sessions;

void netIOProcess();
void netProc_Accept();
void netProc_Recv(SOCKETINFO* pSession);
void netProc_Send(SOCKETINFO* pSession);
void Disconnect(SOCKETINFO* pSession);
// 브로드캐스트(발신자 제외)
void BroadcastPacketExcept(SOCKETINFO* exclude, CPacket* pPacket);
// 브로드캐스트(발신자 포함)
// 나중에 세션이 가진 범위의 세션에게만 전달
void BroadcastPacket(SOCKETINFO* psession, CPacket* pPacket);

void SendPacket_Around(SOCKETINFO* psession, CPacket* pPacket, bool self);
void SendPacketToSession(SOCKETINFO* pSession, const void* data, int len);
void UpdateLogic(double dt);

bool PacketProc(SOCKETINFO* pSession, unsigned char byPacketType, CPacket*& Packet);
bool SendPacket(SOCKETINFO* pSession, unsigned char byPacketType);

bool netPacketProc_MoveStart(SOCKETINFO* pSession, CPacket* pPacket);
bool netPacketProc_MoveStop(SOCKETINFO* pSession, CPacket* pPacket);
bool netPacketProc_Attack1(SOCKETINFO* pSession, CPacket* pPacket);
bool netPacketProc_Attack2(SOCKETINFO* pSession, CPacket* pPacket);
bool netPacketProc_Attack3(SOCKETINFO* pSession, CPacket* pPacket);

bool clientPacketProc_CREATE_MY_CHARACTER(SOCKETINFO* pSession, CPacket* newPacket);
bool clientPacketProc_CREATE_OTHER_CHARACTER(SOCKETINFO* pSession, CPacket* newPacket);
//bool clientPacketProc_ATTACK1(SOCKETINFO* pSession, CPacket* newPacket);
//bool clientPacketProc_ATTACK2(SOCKETINFO* pSession, CPacket* newPacket);
//bool clientPacketProc_ATTACK3(SOCKETINFO* pSession, CPacket* newPacket);
//bool clientPacketProc_DAMAGE(SOCKETINFO* pSession, CPacket* newPacket);
//bool clientPacketProc_DELETE_CHARACTER(SOCKETINFO* pSession, CPacket* newPacket);
//bool clientPacketProc_MOVE_START(SOCKETINFO* pSession, CPacket* newPacket);
//bool clientPacketProc_MOVE_STOP(SOCKETINFO* pSession, CPacket* newPacket);

void mpCreateOtherCharater(CPacket* pPacket, DWORD dwSessionID, BYTE byDir, short shX, short shY, BYTE hp);
void mpDamage(CPacket* pPacket, DWORD dwAttackerSessionID, DWORD dwDamagerID, BYTE hp);
void mpAttack(CPacket* pPacket, DWORD dwSessionID, BYTE byDir, short shX, short shY, char attackType);
void mpMoveStart(CPacket* pPacket, DWORD dwSessionID, BYTE byDir, short shX, short shY);
void mpMoveStop(CPacket* pPacket, DWORD dwSessionID, BYTE byDir, short shX, short shY);
void mpSync(CPacket* pPacket, DWORD dwSessionID, short shX, short shY);



int main() {
    timeBeginPeriod(1);

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        cout << "WSAStartup failed\n";
        return -1;
    }

    g_ListenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (g_ListenSocket == INVALID_SOCKET) {
        cout << "socket failed\n";
        WSACleanup();
        return -1;
    }

    // 논블로킹
    u_long on = 1;
    ioctlsocket(g_ListenSocket, FIONBIO, &on);

    sockaddr_in srv;
    ZeroMemory(&srv, sizeof(srv));
    srv.sin_family = AF_INET;
    srv.sin_addr.s_addr = htonl(INADDR_ANY);
    srv.sin_port = htons(dfNETWORK_PORT);

    if (bind(g_ListenSocket, (sockaddr*)&srv, sizeof(srv)) == SOCKET_ERROR) {
        cerr << "bind failed\n";
        closesocket(g_ListenSocket);
        WSACleanup();
        return -1;
    }

    if (listen(g_ListenSocket, SOMAXCONN) == SOCKET_ERROR) {
        cerr << "listen failed\n";
        closesocket(g_ListenSocket);
        WSACleanup();
        return -1;
    }

    cout << "Server listening on port " << dfNETWORK_PORT << "\n";

    // 메인 루프
    using clock = std::chrono::high_resolution_clock;
    auto lastLogic = clock::now();
    const double logicInterval = 1.0 / TICKS_PER_SEC;

    while (!g_bShutdown) {
        netIOProcess();

        //프레임
        auto now = clock::now();
        std::chrono::duration<double> elapsed = now - lastLogic;
        if (elapsed.count() >= logicInterval) {
            UpdateLogic(elapsed.count());
            lastLogic = now;
        }

        //Sleep(1);
    }

    closesocket(g_ListenSocket);
    WSACleanup();
    timeEndPeriod(1);
    return 0;
}

// --------------------- 네트워크 I/O ---------------------
void netIOProcess() {

    long long quotientOfDivide64;
    if (g_sessionMap->GetSize() % 64 == 0)
    {
        quotientOfDivide64 = g_sessionMap->GetSize() / 64;
    }
    else
    {
        quotientOfDivide64 = g_sessionMap->GetSize() / 64 + 1;
    }

    for (long long i = 0; i < quotientOfDivide64; i++)
    {
        FD_SET readSet, writeSet;
        FD_ZERO(&readSet);
        FD_ZERO(&writeSet);

        FD_SET(g_ListenSocket, &readSet);

        for (long long j = 0; j < 64; j++)
        {
            long long index = 64 * i + j;
            SOCKETINFO* s;
            g_sessionMap->GetSessionptr(index, s);
            if (s && s->sock != INVALID_SOCKET) {
                if (s->sendBuf->GetUseSize() > 0) FD_SET(s->sock, &writeSet);
                FD_SET(s->sock, &readSet);
            }
        }

        timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 0;
        int ret = select(0, &readSet, &writeSet, nullptr, &tv);
        if (ret == SOCKET_ERROR) {
            int err = WSAGetLastError();
            cerr << "select error: " << err << "\n";
            return;
        }
        if (ret <= 0) continue;

        if (FD_ISSET(g_ListenSocket, &readSet)) {
            netProc_Accept();
        }

        // 각 세션 체크
        for (long long j = 0; j < 64; j++)
        {
            long long index = 64 * i + j;
            SOCKETINFO* s;
            g_sessionMap->GetSessionptr(index, s);
            if (s && s->sock != INVALID_SOCKET) {
                if (FD_ISSET(s->sock, &writeSet)) {
                    netProc_Send(s);
                }
                if (FD_ISSET(s->sock, &readSet)) {
                    netProc_Recv(s);
                }
            }
        }

    }
}

// accept
void netProc_Accept() {
    sockaddr_in clientAddr;
    int addrlen = sizeof(clientAddr);
    SOCKET clientSock = accept(g_ListenSocket, (sockaddr*)&clientAddr, &addrlen);
    if (clientSock == INVALID_SOCKET) {
        int e = WSAGetLastError();
        if (e != WSAEWOULDBLOCK) cerr << "accept failed: " << e << "\n";
        return;
    }

    // 논블로킹으로
    u_long on = 1;
    ioctlsocket(clientSock, FIONBIO, &on);

    //64 명이 싹다 Mss크기로 보밸 수 있는 경우 최대 버퍼 크기
    SOCKETINFO* s = new SOCKETINFO(4 * 1024);
    long long id = g_sessionMap->AddSession(s);
    s->sock = clientSock;
    s->session_id = id;
   

    cout << "Accepted new client (session " << s->session_id << ")\n";
    
    
    
    //신규 클라이언트에게 자기 캐릭터 할당 패킷 전송
    SendPacket(s, dfPACKET_SC_CREATE_MY_CHARACTER);

    //기존 접속자 정보를 신규 클라이언트에게 전송 (SC_CREATE_OTHER_CHARACTER)
    SendPacket(s, dfPACKET_SC_CREATE_OTHER_CHARACTER);


    CPacket* pPacket = new CPacket;
    mpCreateOtherCharater(pPacket, s->session_id, s->byDirection, s->shX, s->shY, s->chHP);
    //다른 클라이언트들에게 신규 접속자 정보 전송
    SendPacket_Around(s, pPacket, false);
    delete pPacket;
}

// recv
void netProc_Recv(SOCKETINFO* pSession) {
    if (!pSession) return;
    int ret = recv(pSession->sock, pSession->recvBuf->GetRearBufferPtr(), pSession->recvBuf->GetFreeSize(), 0);
    if (ret == 0) {
        cout << "Client closed (session " << pSession->session_id << ")\n";
        Disconnect(pSession);
        return;
    }
    else if (ret == SOCKET_ERROR) {
        int e = WSAGetLastError();
        if (e == WSAEWOULDBLOCK) {
             //수신버퍼에 읽을게 없음
            return;
        }
        else
        {
            cerr << "recv error (" << pSession->session_id << "): " << e << "\n";
            Disconnect(pSession);
            return;
        }
    }
    else {
        // 링버퍼에 저장 (Enqueue)
        pSession->recvBuf->MoveRear(ret);
    }

    CPacket* pPacket = new CPacket;
    
    // 완성된 패킷이 있으면 처리
    while (pSession->recvBuf->GetUseSize() >= (int)sizeof(st_PACKET_HEADER)) {

        st_PACKET_HEADER hdr;
        int peeked = pSession->recvBuf->Peek((char*)&hdr, (int)sizeof(hdr));
        if (peeked < (int)sizeof(hdr)) break; // 안전
        if (hdr.byCode != dfPACKET_CODE) {
            while (1)
            {
                cerr << "[net_Recv] Bad packet code from session " << pSession->session_id << "\n";
            }
            Disconnect(pSession);
            return;
        }
        if (hdr.bySize < sizeof(st_PACKET_HEADER) || hdr.bySize > 255) {
            while (1)
            {
                cerr << "[net_Recv] Bad packet size from session " << pSession->session_id << ": " << (int)hdr.bySize << "\n";
            }
            Disconnect(pSession);
            return;
        }
        // 아직 전체 도착 안함
        if (pSession->recvBuf->GetUseSize() < (int)sizeof(hdr) + hdr.bySize) break;

        // 전체 패킷을 꺼내서 처리 (Dequeue)
        char buf[100];
        int got = pSession->recvBuf->Dequeue(buf, hdr.bySize + sizeof(st_PACKET_HEADER));
        if (got != hdr.bySize + sizeof(st_PACKET_HEADER)) {
            while (1)
            {
                cerr << "[net_Recv] Dequeue size mismatch\n";
            }
            Disconnect(pSession);
            return;
        }

        pPacket->Clear();
        int ret = pPacket->PutData(buf, hdr.bySize + sizeof(st_PACKET_HEADER));
        if (ret != hdr.bySize + sizeof(st_PACKET_HEADER))
        {
            while (1)
            {
                cerr << "[net_Recv] PutData size mismatch\n";
            }
        }
        
        //헤더의 길이를 제외한  payload만 넘겨준다.
        ret = pPacket->MoveReadPos(sizeof(st_PACKET_HEADER));
        if (ret != sizeof(st_PACKET_HEADER))
        {
            while (1)
            {
                cerr << "[net_Recv] MoveReadPos size mismatch\n";
            }
        }

        if (!PacketProc(pSession, hdr.byType,pPacket)) {
            // 처리 실패 시 연결 종료
            while (1)
            {
                cerr << "[net_Recv] Message Excute Fail\n";
            }
            Disconnect(pSession);
            return;
        }
    }
    delete pPacket;
}

// send: SendQ -> 실제 send (Peek -> send -> Dequeue(sent))
void netProc_Send(SOCKETINFO* pSession) {
    if (!pSession) return;
    int avail = pSession->sendBuf->GetUseSize();
    if (avail <= 0) return;

    int sent = send(pSession->sock, pSession->sendBuf->GetFrontBufferPtr(), avail, 0);
    if (sent == SOCKET_ERROR) {
        int e = WSAGetLastError();
        if (e == WSAEWOULDBLOCK) {
            while (1)
            {
                cout << "[Send] 송신 버퍼에 집어넣을 수 없는 상황\n";
            }
            // 아무 것도 제거하지 않음 (데이터는 아직 SendQ에 있음)
            return;
        }
        else {
            cerr << "send error (" << pSession->session_id << "): " << e << "\n";
            Disconnect(pSession);
            return;
        }
    }
    else if (sent > 0) {
        int dec = pSession->sendBuf->Dequeue(pSession->sendBuf->GetFrontBufferPtr(), sent);

        if (dec != sent) {
            while (1)
            {
                cout << "[Send] 송신 버퍼에서 끄집어낸 크기가 달라짐\n";
            }
        }
    }
}

// Disconnect
void Disconnect(SOCKETINFO* pSession) {
    if (!pSession) return;
    cout << "Disconnect session " << pSession->session_id << "\n";

    //모든 클라이언트에게 삭제 패킷 전송
    CPacket* pPacket = new CPacket;
    st_PACKET_HEADER hdr;
    hdr.byCode = dfPACKET_CODE;
    hdr.bySize = sizeof(st_SC_DELETE_CHARACTER);
    hdr.byType = dfPACKET_SC_DELETE_CHARACTER;

    *pPacket << hdr.byCode;
    *pPacket << hdr.bySize;
    *pPacket << hdr.byType;

    *pPacket << (long) pSession->session_id;

    //어차피 본인은 못받음.
    BroadcastPacketExcept(pSession, pPacket);

    //클라이언트 정보 얻기
    SOCKADDR_IN clientaddr;
    int addrlen = sizeof(clientaddr);
    getpeername(pSession->sock, (SOCKADDR*)&clientaddr, &addrlen);

    // 서버에서 삭제
    g_sessionMap->deleteSession(pSession, inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
}



// 브로드캐스트(발신자 포함)
// 나중에 세션이 가진 범위의 세션에게만 전달
void BroadcastPacket(SOCKETINFO* psession, CPacket* pPacket) {
    for (long long i = 0; i < g_sessionMap->GetSize();i++) {
        SOCKETINFO* s;
        g_sessionMap->GetSessionptr(i, s);
        if (s == nullptr) continue;
        s->sendBuf->Enqueue((char*)pPacket, pPacket->GetDataSize());
    }
}

// 브로드캐스트(발신자 제외)
void BroadcastPacketExcept(SOCKETINFO* exclude, CPacket* pPacket) {
    for (long long i = 0; i < g_sessionMap->GetSize();i++) {
        SOCKETINFO* s;
        g_sessionMap->GetSessionptr(i, s);
        if (s == nullptr || s == exclude) continue;
        s->sendBuf->Enqueue((char*)pPacket, pPacket->GetDataSize());
    }
}

void SendPacket_Around(SOCKETINFO* psession, CPacket* pPacket, bool self)
{
    if (self)
    {
        for (long long i = 0; i < g_sessionMap->GetSize();i++) {
            SOCKETINFO* s;
            g_sessionMap->GetSessionptr(i, s);
            if (s == nullptr) continue;
            s->sendBuf->Enqueue((char*)pPacket, pPacket->GetDataSize());
        }
    }
    else
    {
        for (long long i = 0; i < g_sessionMap->GetSize();i++) {
            SOCKETINFO* s;
            g_sessionMap->GetSessionptr(i, s);
            if (s == nullptr || s == psession) continue;
            s->sendBuf->Enqueue((char*)pPacket, pPacket->GetDataSize());
        }
    }

}

// 특정 세션에 패킷 전송(큐에 저장)
void SendPacketToSession(SOCKETINFO* pSession, const void* data, int len) {
    if (!pSession) return;
    pSession->sendBuf->Enqueue((char*)data, len);
}


// -------------------- 게임 로직 --------------------
void UpdateLogic(double dt) {
    (void)dt;
    // 각 세션의 이동 처리: byDirection을 가지고 단순 이동 처리
    for (int i = 0; i < g_sessionMap->GetSize();i++) {
        SOCKETINFO* s;
        g_sessionMap->GetSessionptr(i, s);
        if (s == nullptr)
        {
            continue;
        }

        if (s->chHP <= 0) {
            cout << "Dead Client Session ID : " << s->session_id << " Client X: " << s->shX << " Y: " << s->shY << " HP:" << (int)s->chHP << "\n";
            Disconnect(s);
            continue;
        }

        if (s->dwAction == 1) {
            switch (s->byDirection) {
            case dfPACKET_MOVE_DIR_LL:
                if (s->shX > dfRANGE_MOVE_LEFT)
                {
                    s->shX = (unsigned short)max((int)dfRANGE_MOVE_LEFT, (int)s->shX - MOVE_UNIT_X);
                    cout << "# gameRun:RR # SessionID:" << s->session_id << " / X:" << s->shX << " / Y:" << s->shY << "\n";
                }
                else s->byDirection = dfPACKET_MOVE_DIR_LL; // 멈춤 처리
                break;
            case dfPACKET_MOVE_DIR_LU:
                if (s->shX > dfRANGE_MOVE_LEFT && s->shY > dfRANGE_MOVE_TOP) {
                    s->shX = (unsigned short)max((int)dfRANGE_MOVE_LEFT, (int)s->shX - MOVE_UNIT_X);
                    s->shY = (unsigned short)max((int)dfRANGE_MOVE_TOP, (int)s->shY - MOVE_UNIT_Y);
                }
                else s->byDirection = dfPACKET_MOVE_DIR_LU; // 멈춤 처리
                break;
            case dfPACKET_MOVE_DIR_UU:
                if (s->shY > dfRANGE_MOVE_TOP)
                {
                    s->shY = (unsigned short)max((int)dfRANGE_MOVE_TOP, (int)s->shY - MOVE_UNIT_Y);
                    cout << "# gameRun:RR # SessionID:" << s->session_id << " / X:" << s->shX << " / Y:" << s->shY << "\n";
                }
                else s->byDirection = dfPACKET_MOVE_DIR_UU;
                break;
            case dfPACKET_MOVE_DIR_RU:
                if (s->shX < dfRANGE_MOVE_RIGHT && s->shY > dfRANGE_MOVE_TOP) {
                    s->shX = (unsigned short)min((int)dfRANGE_MOVE_RIGHT, (int)s->shX + MOVE_UNIT_X);
                    s->shY = (unsigned short)max((int)dfRANGE_MOVE_TOP, (int)s->shY - MOVE_UNIT_Y);
                }
                else s->byDirection = dfPACKET_MOVE_DIR_RU;
                break;
            case dfPACKET_MOVE_DIR_RR:
                if (s->shX < dfRANGE_MOVE_RIGHT)
                {
                    s->shX = (unsigned short)min((int)dfRANGE_MOVE_RIGHT, (int)s->shX + MOVE_UNIT_X);
                    cout << "# gameRun:RR # SessionID:" << s->session_id << " / X:" << s->shX << " / Y:" << s->shY << "\n";
                }
                else s->byDirection = dfPACKET_MOVE_DIR_RR;
                break;
            case dfPACKET_MOVE_DIR_RD:
                if (s->shX < dfRANGE_MOVE_RIGHT && s->shY < dfRANGE_MOVE_BOTTOM) {
                    s->shX = (unsigned short)min((int)dfRANGE_MOVE_RIGHT, (int)s->shX + MOVE_UNIT_X);
                    s->shY = (unsigned short)min((int)dfRANGE_MOVE_BOTTOM, (int)s->shY + MOVE_UNIT_Y);
                }
                else s->byDirection = dfPACKET_MOVE_DIR_RD;
                break;

            case dfPACKET_MOVE_DIR_DD:
                if (s->shY < dfRANGE_MOVE_BOTTOM) {
                    cout << "# gameRun:RR # SessionID:" << s->session_id << " / X:" << s->shX << " / Y:" << s->shY << "\n";
                    s->shY = (unsigned short)min((int)dfRANGE_MOVE_BOTTOM, (int)s->shY + MOVE_UNIT_Y);
                }
                else s->byDirection = dfPACKET_MOVE_DIR_DD;
                break;
            case dfPACKET_MOVE_DIR_LD:
                if (s->shX > dfRANGE_MOVE_LEFT && s->shY < dfRANGE_MOVE_BOTTOM) {
                    s->shX = (unsigned short)max((int)dfRANGE_MOVE_LEFT, (int)s->shX - MOVE_UNIT_X);
                    s->shY = (unsigned short)min((int)dfRANGE_MOVE_BOTTOM, (int)s->shY + MOVE_UNIT_Y);
                }
                else s->byDirection = dfPACKET_MOVE_DIR_LD;
                break;
            default:
                break;
            }
        }
    }
}

bool SendPacket(SOCKETINFO* pSession, unsigned char byPacketType)
{
    CPacket* newPacket = new CPacket;
    st_PACKET_HEADER hdr;
    hdr.byCode = dfPACKET_CODE;

    switch (byPacketType)
    {
    case dfPACKET_SC_CREATE_MY_CHARACTER:
        hdr.bySize = sizeof(st_SC_CREATE_MY_CHARACTER);
        hdr.byType = dfPACKET_SC_CREATE_MY_CHARACTER;
        *newPacket << hdr.byCode;
        *newPacket << hdr.bySize;
        *newPacket << hdr.byType;
        return clientPacketProc_CREATE_MY_CHARACTER(pSession, newPacket);
        break;

    case dfPACKET_SC_CREATE_OTHER_CHARACTER:
        hdr.bySize = sizeof(st_SC_CREATE_OTHER_CHARACTER);
        hdr.byType = dfPACKET_SC_CREATE_OTHER_CHARACTER;
        *newPacket << hdr.byCode;
        *newPacket << hdr.bySize;
        *newPacket << hdr.byType;
        return clientPacketProc_CREATE_OTHER_CHARACTER(pSession, newPacket);
        break;

  /*  case dfPACKET_SC_ATTACK1:
        hdr.bySize = sizeof(st_SC_ATTACK);
        hdr.byType = dfPACKET_SC_ATTACK1;
        *newPacket << hdr.byCode;
        *newPacket << hdr.bySize;
        *newPacket << hdr.byType;
        return clientPacketProc_ATTACK1(pSession, newPacket);
        break;

    case dfPACKET_SC_ATTACK2:
        hdr.bySize = sizeof(st_SC_ATTACK);
        hdr.byType = dfPACKET_SC_ATTACK2;
        *newPacket << hdr.byCode;
        *newPacket << hdr.bySize;
        *newPacket << hdr.byType;
        return clientPacketProc_ATTACK2(pSession, newPacket);
        break;

    case dfPACKET_SC_ATTACK3:
        hdr.bySize = sizeof(st_SC_ATTACK);
        hdr.byType = dfPACKET_SC_ATTACK3;
        *newPacket << hdr.byCode;
        *newPacket << hdr.bySize;
        *newPacket << hdr.byType;
        return clientPacketProc_ATTACK3(pSession, newPacket);
        break;

    case dfPACKET_SC_DAMAGE:
        hdr.bySize = sizeof(st_SC_DAMAGE);
        hdr.byType = dfPACKET_SC_DAMAGE;
        *newPacket << hdr.byCode;
        *newPacket << hdr.bySize;
        *newPacket << hdr.byType;
        return clientPacketProc_DAMAGE(pSession, newPacket);
        break;*/

    //case dfPACKET_SC_DELETE_CHARACTER:
    //    hdr.bySize = sizeof(st_SC_DELETE_CHARACTER);
    //    hdr.byType = dfPACKET_SC_DELETE_CHARACTER;
    //    *newPacket << hdr.byCode;
    //    *newPacket << hdr.bySize;
    //    *newPacket << hdr.byType;
    //    return clientPacketProc_DELETE_CHARACTER(pSession, newPacket);
    //    break;

   /* case dfPACKET_SC_MOVE_START:
        hdr.bySize = sizeof(st_SC_MOVE_START);
        hdr.byType = dfPACKET_SC_MOVE_START;
        *newPacket << hdr.byCode;
        *newPacket << hdr.bySize;
        *newPacket << hdr.byType;
        return clientPacketProc_MOVE_START(pSession, newPacket);
        break;

    case dfPACKET_SC_MOVE_STOP:
        hdr.bySize = sizeof(st_SC_MOVE_STOP);
        hdr.byType = dfPACKET_SC_MOVE_STOP;
        *newPacket << hdr.byCode;
        *newPacket << hdr.bySize;
        *newPacket << hdr.byType;
        return clientPacketProc_MOVE_STOP(pSession, newPacket);
        break;*/
    }
    return TRUE;
}

bool PacketProc(SOCKETINFO* pSession, unsigned char byPacketType, CPacket*& pPacket)
{

    switch (byPacketType)
    {
    case dfPACKET_CS_MOVE_START:
        return netPacketProc_MoveStart(pSession, pPacket);
        break;

    case dfPACKET_CS_MOVE_STOP:
        return netPacketProc_MoveStop(pSession, pPacket);
        break;

    case dfPACKET_CS_ATTACK1:
        return netPacketProc_Attack1(pSession, pPacket);
        break;

    case dfPACKET_CS_ATTACK2:
        return netPacketProc_Attack2(pSession, pPacket);
        break;

    case dfPACKET_CS_ATTACK3:
        return netPacketProc_Attack3(pSession, pPacket);
        break;
    default:
        while (1)
        {
            cerr << "Unknown packet type from " << pSession->session_id << ": " << (int)byPacketType << "\n";
        }
        break;
    }
    return TRUE;
}

bool netPacketProc_MoveStart(SOCKETINFO* pSession, CPacket* pPacket)
{
    if (pPacket->GetDataSize() < (int)sizeof(st_CS_MOVE_START)) return false;
    
    BYTE byDirection;
    short shX, shY;

    *pPacket >> byDirection;
    *pPacket >> shX;
    *pPacket >> shY;


    //_LOG(dfLOG_LEVEL_DEBUG, L"# MOVESTART # SessionID:%d / Direction:%d / X:%d / Y:%d",
    //    pSession->dwSessionID, byDirection, shX, shY);

    // 위치 오차 검사
    if ((int)abs((int)pSession->shX - (int)shX) > dfERROR_RANGE ||
        (int)abs((int)pSession->shY - (int)shY) > dfERROR_RANGE) {
        mpSync(pPacket, pSession->session_id, pSession->shX, pSession->shY);
        SendPacket_Around(pSession, pPacket, true);

        shX = pSession->shX;
        shY = pSession->shY;
    }

    // 상태 갱신
    //----------------------------------------------------- 
    // 동작을 변경.  동작번호와, 방향값이 같다. 
    //----------------------------------------------------- 
    pSession->dwAction = byDirection;

    //----------------------------------------------------- 
    // 단순 방향표시용 byDirection (LL,RR) 과  
    // 이동시 8방향 (LL,LU,UU,RU,RR,RD,DD,LD) 용 MoveDirection 이 있음. 
    //----------------------------------------------------- 
    pSession->byMoveDirection = byDirection;

    //----------------------------------------------------- 
     // 방향을 변경. 
    //----------------------------------------------------- 
    switch (byDirection)
    {
    case dfPACKET_MOVE_DIR_RR:
    case dfPACKET_MOVE_DIR_RU:
    case dfPACKET_MOVE_DIR_RD:
        pSession->byDirection = dfPACKET_MOVE_DIR_RR;
        break;

    case dfPACKET_MOVE_DIR_LU:
    case dfPACKET_MOVE_DIR_LL:
    case dfPACKET_MOVE_DIR_LD:
        pSession->byDirection = dfPACKET_MOVE_DIR_LL;
        break;
    }
    pSession->shX = shX;
    pSession->shY = shY;

    // 정지를 하면서 좌표가 약간 조절된 경우섹터 업데이트를 함.  (섹터는 차후 설명) 
    //----------------------------------------------------------------------- 
    //if (Sector_UpdateCharacter(pCharacter))
    //{
    //    //----------------------------------------------------------------------- 
    //    // 섹터가 변경된 경우는 클라에게 관련 정보를 쏜다. (섹터는 차후 설명) 
    //    //----------------------------------------------------------------------- 
    //    CharacterSectorUpdatePacket(pCharacter);
    //}
    //----------------------------------------------------------------------- 

    mpMoveStart(pPacket, pSession->session_id, byDirection, pSession->shX, pSession->shY);


    //----------------------------------------------------- 
    // 현재 접속중인 사용자에게 모든 패킷을 뿌린다. (섹터 단위 패킷 전송 함수 ) 
    //----------------------------------------------------- 
    SendPacket_Around(pSession, pPacket, true);
    return true;
}

bool netPacketProc_MoveStop(SOCKETINFO* pSession, CPacket* pPacket)
{
    if (pPacket->GetDataSize() < (int)sizeof(st_CS_MOVE_START)) return false;

    BYTE byDirection;
    short shX, shY;

    *pPacket >> byDirection;
    *pPacket >> shX;
    *pPacket >> shY;

    //_LOG(dfLOG_LEVEL_DEBUG, L"# MOVESTART # SessionID:%d / Direction:%d / X:%d / Y:%d",
   //    pSession->dwSessionID, byDirection, shX, shY);

   // 위치 오차 검사
    if ((int)abs((int)pSession->shX - (int)shX) > dfERROR_RANGE ||
        (int)abs((int)pSession->shY - (int)shY) > dfERROR_RANGE) {
        mpSync(pPacket, pSession->session_id, pSession->shX, pSession->shY);
        SendPacket_Around(pSession, pPacket, true);

        shX = pSession->shX;
        shY = pSession->shY;
    }

    // 상태 갱신
    //----------------------------------------------------- 
    // 동작을 변경.  동작번호와, 방향값이 같다. 
    //----------------------------------------------------- 
    pSession->dwAction = byDirection;

    //----------------------------------------------------- 
    // 단순 방향표시용 byDirection (LL,RR) 과  
    // 이동시 8방향 (LL,LU,UU,RU,RR,RD,DD,LD) 용 MoveDirection 이 있음. 
    //----------------------------------------------------- 
    pSession->byMoveDirection = byDirection;

    //----------------------------------------------------- 
     // 방향을 변경. 
    //----------------------------------------------------- 
    switch (byDirection)
    {
    case dfPACKET_MOVE_DIR_RR:
    case dfPACKET_MOVE_DIR_RU:
    case dfPACKET_MOVE_DIR_RD:
        pSession->byDirection = dfPACKET_MOVE_DIR_RR;
        break;

    case dfPACKET_MOVE_DIR_LU:
    case dfPACKET_MOVE_DIR_LL:
    case dfPACKET_MOVE_DIR_LD:
        pSession->byDirection = dfPACKET_MOVE_DIR_LL;
        break;
    }
    pSession->shX = shX;
    pSession->shY = shY;

    // 정지를 하면서 좌표가 약간 조절된 경우섹터 업데이트를 함.  (섹터는 차후 설명) 
    //----------------------------------------------------------------------- 
    //if (Sector_UpdateCharacter(pCharacter))
    //{
    //    //----------------------------------------------------------------------- 
    //    // 섹터가 변경된 경우는 클라에게 관련 정보를 쏜다. (섹터는 차후 설명) 
    //    //----------------------------------------------------------------------- 
    //    CharacterSectorUpdatePacket(pCharacter);
    //}
    //----------------------------------------------------------------------- 

    mpMoveStop(pPacket, pSession->session_id, byDirection, pSession->shX, pSession->shY);


    //----------------------------------------------------- 
    // 현재 접속중인 사용자에게 모든 패킷을 뿌린다. (섹터 단위 패킷 전송 함수 ) 
    //----------------------------------------------------- 
    SendPacket_Around(pSession, pPacket, true);
    return true;
}

bool netPacketProc_Attack1(SOCKETINFO* pSession, CPacket* pPacket)
{
    if (pPacket->GetDataSize() < (int)sizeof(st_CS_MOVE_START)) return false;

    BYTE byDirection;
    short shX, shY;

    *pPacket >> byDirection;
    *pPacket >> shX;
    *pPacket >> shY;

    //_LOG(dfLOG_LEVEL_DEBUG, L"# MOVESTART # SessionID:%d / Direction:%d / X:%d / Y:%d",
   //    pSession->dwSessionID, byDirection, shX, shY);

   // 위치 오차 검사
    if ((int)abs((int)pSession->shX - (int)shX) > dfERROR_RANGE ||
        (int)abs((int)pSession->shY - (int)shY) > dfERROR_RANGE) {
        mpSync(pPacket, pSession->session_id, pSession->shX, pSession->shY);
        SendPacket_Around(pSession, pPacket, true);

        shX = pSession->shX;
        shY = pSession->shY;
    }

    // 상태 갱신
     //----------------------------------------------------- 
     // 동작을 변경.  동작번호와, 방향값이 같다. ??
     //----------------------------------------------------- 
     pSession->dwAction = byDirection;

    //----------------------------------------------------- 
    // 단순 방향표시용 byDirection (LL,RR) 과  
    // 이동시 8방향 (LL,LU,UU,RU,RR,RD,DD,LD) 용 MoveDirection 이 있음. 
    //----------------------------------------------------- 
    pSession->byMoveDirection = byDirection;

    //----------------------------------------------------- 
     // 방향을 변경. 
    //----------------------------------------------------- 
    switch (byDirection)
    {
    case dfPACKET_MOVE_DIR_RR:
    case dfPACKET_MOVE_DIR_RU:
    case dfPACKET_MOVE_DIR_RD:
        pSession->byDirection = dfPACKET_MOVE_DIR_RR;
        break;

    case dfPACKET_MOVE_DIR_LU:
    case dfPACKET_MOVE_DIR_LL:
    case dfPACKET_MOVE_DIR_LD:
        pSession->byDirection = dfPACKET_MOVE_DIR_LL;
        break;
    }
    pSession->shX = shX;
    pSession->shY = shY;

    // 공격 범위 내 충돌 검사 및 데미지 처리
    int ATTACK_RANGE_X = dfATTACK1_RANGE_X;
    int ATTACK_RANGE_Y = dfATTACK1_RANGE_Y;

    //지금은 전체 플레이어에게 전송
    //섹터안에 있는 플레이어들 중 공격범위 안에 있는 얘들한테 전송해야함.
    for (int i = 0; i < g_sessionMap->GetSize();i++)
    {
        SOCKETINFO* target;
        g_sessionMap->GetSessionptr(i, target);
        if (target == nullptr || target == pSession) continue;
        if (target->chHP <= 0) continue;

        bool rangeDirection = false;
        if (((int)target->shX >= (int)pSession->shX && pSession->byDirection == dfPACKET_MOVE_DIR_RR) ||
            ((int)target->shX <= (int)pSession->shX && pSession->byDirection == dfPACKET_MOVE_DIR_LL))
        {
            rangeDirection = true;
        }

        unsigned int dx = abs((int)target->shX - (int)pSession->shX);
        unsigned int dy = abs((int)target->shY - (int)pSession->shY);

        if (dx <= ATTACK_RANGE_X && rangeDirection && dy <= ATTACK_RANGE_Y) {
            target->chHP -= 10;

            pPacket->Clear();
            mpDamage(pPacket, pSession->session_id, target->session_id, target->chHP);
            SendPacket_Around(target, pPacket, true);

            cout << "Damged Client Session ID : " << target->session_id << " Client X: " << target->shX << " Y: " << target->shY << " HP:" << (int)target->chHP << "\n";
        }
    }

    pPacket->Clear();
    mpAttack(pPacket, pSession->session_id, pSession->byDirection, 
        pSession->shX, pSession->shY, dfPACKET_CS_ATTACK1);
    SendPacket_Around(pSession, pPacket, true);
    cout << " # PACKET_ATTACK : " << "Attack Client Session ID : " << pSession->session_id << " Client X: " << pSession->shX << " Y: " << pSession->shY << "\n";

    return true;
}

bool netPacketProc_Attack2(SOCKETINFO* pSession, CPacket* pPacket)
{
    if (pPacket->GetDataSize() < (int)sizeof(st_CS_MOVE_START)) return false;

    BYTE byDirection;
    short shX, shY;

    *pPacket >> byDirection;
    *pPacket >> shX;
    *pPacket >> shY;

    //_LOG(dfLOG_LEVEL_DEBUG, L"# MOVESTART # SessionID:%d / Direction:%d / X:%d / Y:%d",
   //    pSession->dwSessionID, byDirection, shX, shY);

   // 위치 오차 검사
    if ((int)abs((int)pSession->shX - (int)shX) > dfERROR_RANGE ||
        (int)abs((int)pSession->shY - (int)shY) > dfERROR_RANGE) {
        mpSync(pPacket, pSession->session_id, pSession->shX, pSession->shY);
        SendPacket_Around(pSession, pPacket, true);

        shX = pSession->shX;
        shY = pSession->shY;
    }

    // 상태 갱신
     //----------------------------------------------------- 
     // 동작을 변경.  동작번호와, 방향값이 같다. ??
     //----------------------------------------------------- 
    pSession->dwAction = byDirection;

    //----------------------------------------------------- 
    // 단순 방향표시용 byDirection (LL,RR) 과  
    // 이동시 8방향 (LL,LU,UU,RU,RR,RD,DD,LD) 용 MoveDirection 이 있음. 
    //----------------------------------------------------- 
    pSession->byMoveDirection = byDirection;

    //----------------------------------------------------- 
     // 방향을 변경. 
    //----------------------------------------------------- 
    switch (byDirection)
    {
    case dfPACKET_MOVE_DIR_RR:
    case dfPACKET_MOVE_DIR_RU:
    case dfPACKET_MOVE_DIR_RD:
        pSession->byDirection = dfPACKET_MOVE_DIR_RR;
        break;

    case dfPACKET_MOVE_DIR_LU:
    case dfPACKET_MOVE_DIR_LL:
    case dfPACKET_MOVE_DIR_LD:
        pSession->byDirection = dfPACKET_MOVE_DIR_LL;
        break;
    }
    pSession->shX = shX;
    pSession->shY = shY;

    // 공격 범위 내 충돌 검사 및 데미지 처리
    int ATTACK_RANGE_X = dfATTACK2_RANGE_X;
    int ATTACK_RANGE_Y = dfATTACK2_RANGE_Y;

    //지금은 전체 플레이어에게 전송
    //섹터안에 있는 플레이어들 중 공격범위 안에 있는 얘들한테 전송해야함.
    for (int i = 0; i < g_sessionMap->GetSize();i++)
    {
        SOCKETINFO* target;
        g_sessionMap->GetSessionptr(i, target);
        if (target == nullptr || target == pSession) continue;
        if (target->chHP <= 0) continue;

        bool rangeDirection = false;
        if (((int)target->shX >= (int)pSession->shX && pSession->byDirection == dfPACKET_MOVE_DIR_RR) ||
            ((int)target->shX <= (int)pSession->shX && pSession->byDirection == dfPACKET_MOVE_DIR_LL))
        {
            rangeDirection = true;
        }

        unsigned int dx = abs((int)target->shX - (int)pSession->shX);
        unsigned int dy = abs((int)target->shY - (int)pSession->shY);

        if (dx <= ATTACK_RANGE_X && rangeDirection && dy <= ATTACK_RANGE_Y) {
            target->chHP -= 10;

            pPacket->Clear();
            mpDamage(pPacket, pSession->session_id, target->session_id, target->chHP);
            SendPacket_Around(target, pPacket, true);

            cout << "Damged Client Session ID : " << target->session_id << " Client X: " << target->shX << " Y: " << target->shY << " HP:" << (int)target->chHP << "\n";
        }
    }

    pPacket->Clear();
    mpAttack(pPacket, pSession->session_id, pSession->byDirection,
        pSession->shX, pSession->shY, dfPACKET_CS_ATTACK2);
    SendPacket_Around(pSession, pPacket, true);
    cout << " # PACKET_ATTACK : " << "Attack Client Session ID : " << pSession->session_id << " Client X: " << pSession->shX << " Y: " << pSession->shY << "\n";

    return true;
}

bool netPacketProc_Attack3(SOCKETINFO* pSession, CPacket* pPacket)
{
    if (pPacket->GetDataSize() < (int)sizeof(st_CS_MOVE_START)) return false;

    BYTE byDirection;
    short shX, shY;

    *pPacket >> byDirection;
    *pPacket >> shX;
    *pPacket >> shY;

    //_LOG(dfLOG_LEVEL_DEBUG, L"# MOVESTART # SessionID:%d / Direction:%d / X:%d / Y:%d",
   //    pSession->dwSessionID, byDirection, shX, shY);

   // 위치 오차 검사
    if ((int)abs((int)pSession->shX - (int)shX) > dfERROR_RANGE ||
        (int)abs((int)pSession->shY - (int)shY) > dfERROR_RANGE) {
        mpSync(pPacket, pSession->session_id, pSession->shX, pSession->shY);
        SendPacket_Around(pSession, pPacket, true);

        shX = pSession->shX;
        shY = pSession->shY;
    }

    // 상태 갱신
     //----------------------------------------------------- 
     // 동작을 변경.  동작번호와, 방향값이 같다. ??
     //----------------------------------------------------- 
    pSession->dwAction = byDirection;

    //----------------------------------------------------- 
    // 단순 방향표시용 byDirection (LL,RR) 과  
    // 이동시 8방향 (LL,LU,UU,RU,RR,RD,DD,LD) 용 MoveDirection 이 있음. 
    //----------------------------------------------------- 
    pSession->byMoveDirection = byDirection;

    //----------------------------------------------------- 
     // 방향을 변경. 
    //----------------------------------------------------- 
    switch (byDirection)
    {
    case dfPACKET_MOVE_DIR_RR:
    case dfPACKET_MOVE_DIR_RU:
    case dfPACKET_MOVE_DIR_RD:
        pSession->byDirection = dfPACKET_MOVE_DIR_RR;
        break;

    case dfPACKET_MOVE_DIR_LU:
    case dfPACKET_MOVE_DIR_LL:
    case dfPACKET_MOVE_DIR_LD:
        pSession->byDirection = dfPACKET_MOVE_DIR_LL;
        break;
    }
    pSession->shX = shX;
    pSession->shY = shY;

    // 공격 범위 내 충돌 검사 및 데미지 처리
    int ATTACK_RANGE_X = dfATTACK3_RANGE_X;
    int ATTACK_RANGE_Y = dfATTACK3_RANGE_Y;

    //지금은 전체 플레이어에게 전송
    //섹터안에 있는 플레이어들 중 공격범위 안에 있는 얘들한테 전송해야함.
    for (int i = 0; i < g_sessionMap->GetSize();i++)
    {
        SOCKETINFO* target;
        g_sessionMap->GetSessionptr(i, target);
        if (target == nullptr || target == pSession) continue;
        if (target->chHP <= 0) continue;

        bool rangeDirection = false;
        if (((int)target->shX >= (int)pSession->shX && pSession->byDirection == dfPACKET_MOVE_DIR_RR) ||
            ((int)target->shX <= (int)pSession->shX && pSession->byDirection == dfPACKET_MOVE_DIR_LL))
        {
            rangeDirection = true;
        }

        unsigned int dx = abs((int)target->shX - (int)pSession->shX);
        unsigned int dy = abs((int)target->shY - (int)pSession->shY);

        if (dx <= ATTACK_RANGE_X && rangeDirection && dy <= ATTACK_RANGE_Y) {
            target->chHP -= 10;

            pPacket->Clear();
            mpDamage(pPacket, pSession->session_id, target->session_id, target->chHP);
            SendPacket_Around(target, pPacket, true);

            cout << "Damged Client Session ID : " << target->session_id << " Client X: " << target->shX << " Y: " << target->shY << " HP:" << (int)target->chHP << "\n";
        }
    }

    pPacket->Clear();
    mpAttack(pPacket, pSession->session_id, pSession->byDirection,
        pSession->shX, pSession->shY, dfPACKET_CS_ATTACK3);
    SendPacket_Around(pSession, pPacket, true);
    cout << " # PACKET_ATTACK : " << "Attack Client Session ID : " << pSession->session_id << " Client X: " << pSession->shX << " Y: " << pSession->shY << "\n";

    return true;
}



bool clientPacketProc_CREATE_MY_CHARACTER(SOCKETINFO* pSession, CPacket* newPacket)
{
    *newPacket << pSession->byDirection;
    *newPacket << pSession->chHP;
    *newPacket << (long)pSession->session_id;
    *newPacket << pSession->shX;
    *newPacket << pSession->shY;

    cout << "Create Client Session ID : " << pSession->session_id << " Client X: " << pSession->shX << " Y: " << pSession->shY << "\n";
    int enqdata = pSession->sendBuf->Enqueue((char*)newPacket, newPacket->GetDataSize());
    if (enqdata != newPacket->GetDataSize())
    {
        while (1)
        {
            printf("[SendPacket] 송신 버퍼에 넣기 실패");
        }
    }

    return true;
}

bool clientPacketProc_CREATE_OTHER_CHARACTER(SOCKETINFO* pSession, CPacket* newPacket)
{
    //주변 캐릭터를 대입하도록 바꿔야하지만 일단은 전체에서 받도록

    for (long long i = 0; i < g_sessionMap->GetSize(); i++)
    {
        SOCKETINFO* other;
        g_sessionMap->GetSessionptr(i, other);
        if (other == nullptr || other == pSession) continue;

        *newPacket << other->byDirection;
        *newPacket << other->chHP;
        *newPacket << (long)other->session_id;
        *newPacket << other->shX;
        *newPacket << other->shY;

        int enqdata = pSession->sendBuf->Enqueue((char*)newPacket, newPacket->GetDataSize());
        if (enqdata != newPacket->GetDataSize())
        {
            while (1)
            {
                printf("[SendPacket] 송신 버퍼에 넣기 실패");
            }
        }

        int size = sizeof(st_SC_CREATE_OTHER_CHARACTER);
        newPacket->MoveWritePos(-size);
    }
    return true;
}

//bool clientPacketProc_ATTACK1(SOCKETINFO* pSession, CPacket* newPacket)
//{
//    return false;
//}
//
//bool clientPacketProc_ATTACK2(SOCKETINFO* pSession, CPacket* newPacket)
//{
//    return false;
//}
//
//bool clientPacketProc_ATTACK3(SOCKETINFO* pSession, CPacket* newPacket)
//{
//    return false;
//}
//
//bool clientPacketProc_DAMAGE(SOCKETINFO* pSession, CPacket* newPacket)
//{
//    pSession->sendBuf->Enqueue((char*)newPacket, newPacket->GetDataSize());
//    SendPacket_Around(pSession, newPacket, false);
//    return false;
//}
//
//bool clientPacketProc_DELETE_CHARACTER(SOCKETINFO* pSession, CPacket* newPacket)
//{
//    return false;
//}
//
//bool clientPacketProc_MOVE_START(SOCKETINFO* pSession, CPacket* newPacket)
//{
//    return false;
//}
//
//bool clientPacketProc_MOVE_STOP(SOCKETINFO* pSession, CPacket* newPacket)
//{
//    return false;
//}

void mpCreateOtherCharater(CPacket* pPacket, DWORD dwSessionID, BYTE byDir, short shX, short shY, BYTE hp)
{
    st_PACKET_HEADER stPacketHeader;
    stPacketHeader.byCode = dfPACKET_CODE;
    stPacketHeader.bySize = 10;
    stPacketHeader.byType = dfPACKET_SC_CREATE_OTHER_CHARACTER;
    pPacket->Clear();
    pPacket->PutData((char*)&stPacketHeader, sizeof(st_PACKET_HEADER));
    *pPacket << (long)dwSessionID;
    *pPacket << byDir;
    *pPacket << shX;
    *pPacket << shY;
    *pPacket << hp;
}

void mpDamage(CPacket* pPacket, DWORD dwAttackerSessionID, DWORD dwDamagerID, BYTE hp)
{
    st_PACKET_HEADER stPacketHeader;
    stPacketHeader.byCode = dfPACKET_CODE;
    stPacketHeader.bySize = sizeof(st_SC_DAMAGE);
    stPacketHeader.byType = dfPACKET_SC_DAMAGE;
    pPacket->Clear();
    pPacket->PutData((char*)&stPacketHeader, sizeof(st_PACKET_HEADER));
    *pPacket << (long)dwAttackerSessionID;
    *pPacket << (long)dwDamagerID;
    *pPacket << hp;
}

void mpAttack(CPacket* pPacket, DWORD dwSessionID, BYTE byDir, short shX, short shY, char attackType)
{
    st_PACKET_HEADER stPacketHeader;
    stPacketHeader.byCode = dfPACKET_CODE;
    stPacketHeader.bySize = sizeof(st_SC_ATTACK);
    stPacketHeader.byType = attackType;
    pPacket->Clear();
    pPacket->PutData((char*)&stPacketHeader, sizeof(st_PACKET_HEADER));
    *pPacket << (long)dwSessionID;
    *pPacket << byDir;
    *pPacket << shX;
    *pPacket << shY;
}

void mpMoveStart(CPacket* pPacket, DWORD dwSessionID, BYTE byDir, short shX, short shY)
{
    st_PACKET_HEADER stPacketHeader;
    stPacketHeader.byCode = dfPACKET_CODE;
    stPacketHeader.bySize = 9;
    stPacketHeader.byType = dfPACKET_SC_MOVE_START;
    pPacket->Clear();
    pPacket->PutData((char*)&stPacketHeader, sizeof(st_PACKET_HEADER));
    *pPacket << (long)dwSessionID;
    *pPacket << byDir;
    *pPacket << shX;
    *pPacket << shY;
}

void mpMoveStop(CPacket* pPacket, DWORD dwSessionID, BYTE byDir, short shX, short shY)
{
    st_PACKET_HEADER stPacketHeader;
    stPacketHeader.byCode = dfPACKET_CODE;
    stPacketHeader.bySize = 9;
    stPacketHeader.byType = dfPACKET_SC_MOVE_STOP;
    pPacket->Clear();
    pPacket->PutData((char*)&stPacketHeader, sizeof(st_PACKET_HEADER));
    *pPacket << (long)dwSessionID;
    *pPacket << byDir;
    *pPacket << shX;
    *pPacket << shY;
}

void mpSync(CPacket* pPacket, DWORD dwSessionID, short shX, short shY)
{
    st_PACKET_HEADER stPacketHeader;
    stPacketHeader.byCode = dfPACKET_CODE;
    stPacketHeader.bySize = sizeof(st__SC_SYNC);
    stPacketHeader.byType = dfPACKET_SC_SYNC;
    pPacket->Clear();
    pPacket->PutData((char*)&stPacketHeader, sizeof(st_PACKET_HEADER));
    *pPacket << (long)dwSessionID;
    *pPacket << shX;
    *pPacket << shY;
}
