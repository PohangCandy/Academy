#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "winmm.lib")

#include <WinSock2.h>
#include <Windows.h>
#include <vector>
#include <chrono>
#include <iostream>
#include "CRingBuffer.h"

using namespace std;


// 서버 설정
#define SERVERPORT 5000
//프레임
#define TICKS_PER_SEC 50.0

// 이동 범위
#define dfRANGE_MOVE_TOP    50
#define dfRANGE_MOVE_LEFT   10
#define dfRANGE_MOVE_RIGHT  630
#define dfRANGE_MOVE_BOTTOM 470

// 이동 단위
#define MOVE_UNIT_X 3
#define MOVE_UNIT_Y 2

// 위치 오차 허용 범위
#define dfERROR_RANGE 50

// 패킷 코드 및 타입
#define dfNETWORK_PACKET_CODE 0x89

#define dfPACKET_SC_CREATE_MY_CHARACTER    0
#define dfPACKET_SC_CREATE_OTHER_CHARACTER 1
#define dfPACKET_SC_DELETE_CHARACTER       2

#define dfPACKET_CS_MOVE_START 10
#define dfPACKET_SC_MOVE_START 11

#define dfPACKET_CS_MOVE_STOP 12
#define dfPACKET_SC_MOVE_STOP 13

#define dfPACKET_CS_ATTACK1 20
#define dfPACKET_SC_ATTACK1 21
#define dfPACKET_CS_ATTACK2 22
#define dfPACKET_SC_ATTACK2 23
#define dfPACKET_CS_ATTACK3 24
#define dfPACKET_SC_ATTACK3 25

#define dfPACKET_SC_DAMAGE 30

// 방향 상수
#define dfPACKET_MOVE_DIR_LL 0
#define dfPACKET_MOVE_DIR_LU 1
#define dfPACKET_MOVE_DIR_UU 2
#define dfPACKET_MOVE_DIR_RU 3
#define dfPACKET_MOVE_DIR_RR 4
#define dfPACKET_MOVE_DIR_RD 5
#define dfPACKET_MOVE_DIR_DD 6
#define dfPACKET_MOVE_DIR_LD 7

//---------------------------------------------------------------
// 공격범위.
//---------------------------------------------------------------
#define dfATTACK1_RANGE_X		80
#define dfATTACK2_RANGE_X		90
#define dfATTACK3_RANGE_X		100
#define dfATTACK1_RANGE_Y		10
#define dfATTACK2_RANGE_Y		10
#define dfATTACK3_RANGE_Y		20

#pragma pack(push,1)
struct st_PACKET_HEADER {
    uint8_t byCode;
    uint8_t bySize;
    uint8_t byType;
};
#pragma pack(pop)


#pragma pack(push,1)
struct st_CS_MOVE_START {
    uint8_t byDirection;
    uint16_t shX;
    uint16_t shY;
};
#pragma pack(pop)


#pragma pack(push,1)
struct st_CS_MOVE_STOP {
    uint8_t byDirection;
    uint16_t shX;
    uint16_t shY;
};
#pragma pack(pop)


#pragma pack(push,1)
struct st_CS_ATTACK {
    uint8_t byDirection;
    uint16_t shX;
    uint16_t shY;
};
#pragma pack(pop)

// 세션 구조
struct st_SESSION {
    SOCKET Socket;
    unsigned long dwSessionID;
    CRingBuffer RecvQ;
    CRingBuffer SendQ;

    unsigned long	dwAction;
    char byDirection;
    short shX;
    short shY;

    char chHP;

    //일단 구조체 생성자로 만들고 함수로 변경하자
    st_SESSION(SOCKET s, unsigned long id)
        : Socket(s)
        , dwSessionID(id)
        , RecvQ(64 * 1024)
        , SendQ(64 * 1024)
        , dwAction(0)
        , byDirection(dfPACKET_MOVE_DIR_RR)
        , shX((rand() % (dfRANGE_MOVE_RIGHT - dfRANGE_MOVE_LEFT + 1)) + dfRANGE_MOVE_LEFT)
        , shY((rand() % (dfRANGE_MOVE_BOTTOM - dfRANGE_MOVE_TOP + 1)) + dfRANGE_MOVE_TOP)
        , chHP(100)
    {}
};

// 전역
SOCKET g_ListenSocket = INVALID_SOCKET;
bool g_bShutdown = false;
vector<st_SESSION*> g_Sessions;
uint32_t g_nextSessionID = 1;

// 전방선언
void netIOProcess();
void netProc_Accept();
void netProc_Recv(st_SESSION* pSession);
void netProc_Send(st_SESSION* pSession);
void Disconnect(st_SESSION* pSession);
bool PacketProc(st_SESSION* pSession, uint8_t byPacketType, char* pPayload, int payloadLen);
void BroadcastPacketExcept(st_SESSION* exclude, const void* data, int len);
void SendPacketToSession(st_SESSION* pSession, const void* data, int len);
void UpdateLogic(double dt);

int main() {
    timeBeginPeriod(1);

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        cerr << "WSAStartup failed\n";
        return -1;
    }

    g_ListenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (g_ListenSocket == INVALID_SOCKET) {
        cerr << "socket failed\n";
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
    srv.sin_port = htons(SERVERPORT);

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

    cout << "Server listening on port " << SERVERPORT << "\n";

    // 메인 루프
    using clock = std::chrono::high_resolution_clock;
    auto lastLogic = clock::now();
    const double logicInterval = 1.0 / TICKS_PER_SEC;

    while (!g_bShutdown) {
        netIOProcess();

        auto now = clock::now();
        std::chrono::duration<double> elapsed = now - lastLogic;
        if (elapsed.count() >= logicInterval) {
            UpdateLogic(elapsed.count());
            lastLogic = now;
        }

        //Sleep(1);
    }

    // 정리
    for (auto s : g_Sessions) {
        closesocket(s->Socket);
        delete s;
    }
    g_Sessions.clear();

    closesocket(g_ListenSocket);
    WSACleanup();
    timeEndPeriod(1);
    return 0;
}

// --------------------- 네트워크 I/O ---------------------
void netIOProcess() {
    FD_SET readSet, writeSet;
    FD_ZERO(&readSet);
    FD_ZERO(&writeSet);

    FD_SET(g_ListenSocket, &readSet);

    for (auto s : g_Sessions) {
        if (s && s->Socket != INVALID_SOCKET) {
            FD_SET(s->Socket, &readSet);
            if (s->SendQ.GetUseSize() > 0) FD_SET(s->Socket, &writeSet);
        }
    }

    timeval tv; tv.tv_sec = 0; tv.tv_usec = 0;
    int ret = select(0, &readSet, &writeSet, nullptr, &tv);
    if (ret == SOCKET_ERROR) {
        int err = WSAGetLastError();
        cerr << "select error: " << err << "\n";
        return;
    }
    if (ret <= 0) return;

    // 리슨 소켓
    int MAX_CLIENTS = 64;

    if (FD_ISSET(g_ListenSocket, &readSet)) {
       if((int)g_Sessions.size() < MAX_CLIENTS)
       {
            netProc_Accept();
       }
       else
       {
           // 최대 클라이언트 초과 -> accept 하지 않거나 accept 후 바로 닫기
           SOCKET tempSock = accept(g_ListenSocket, nullptr, nullptr);
           if (tempSock != INVALID_SOCKET) {
               closesocket(tempSock);  // 바로 닫아서 접속 거부
               std::cout << "접속 거부: 최대 클라이언트 초과\n";
           }
       }
    } 

    // 각 세션 체크 (인덱스 순회; Disconnect 시 벡터 변경 가능 -> 주의)
    for (size_t i = 0; i < g_Sessions.size(); ++i) {
        st_SESSION* s = g_Sessions[i];
        if (!s) continue;

        // recv 가능
        if (FD_ISSET(s->Socket, &readSet)) {
            netProc_Recv(s);
        }
        // send 가능
        if (i >= g_Sessions.size()) break; // 안전 검사 (Disconnect로 인덱스 변경 가능)
        s = g_Sessions[i];
        if (!s) continue;
        if (FD_ISSET(s->Socket, &writeSet)) {
            netProc_Send(s);
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

    st_SESSION* s = new st_SESSION(clientSock, g_nextSessionID++);
    g_Sessions.push_back(s);

    cout << "Accepted new client (session " << s->dwSessionID << ")\n";

    //신규 클라이언트에게 자기 캐릭터 할당 패킷 전송 (dfPACKET_SC_CREATE_MY_CHARACTER)
    {
        uint8_t buf[3 + 4 + 1 + 2 + 2 + 1];
        st_PACKET_HEADER hdr;
        hdr.byCode = dfNETWORK_PACKET_CODE;
        hdr.bySize = sizeof(buf) - sizeof(st_PACKET_HEADER);
        hdr.byType = dfPACKET_SC_CREATE_MY_CHARACTER;
        memcpy(buf, &hdr, sizeof(hdr));
        uint32_t id_net = s->dwSessionID;
        memcpy(buf + 3, &id_net, 4);
        buf[7] = s->byDirection;
        uint16_t x_net = s->shX;
        uint16_t y_net = s->shY;
        memcpy(buf + 8, &x_net, 2);
        memcpy(buf + 10, &y_net, 2);
        buf[12] = s->chHP;
        cout << "Create Client Session ID : " << s->dwSessionID << " Client X: " << x_net << " Y: " << y_net << "\n";
        s->SendQ.Enqueue((char*)buf, (int)sizeof(buf));
    }

    //기존 접속자 정보를 신규 클라이언트에게 전송 (SC_CREATE_OTHER_CHARACTER)
    for (auto other : g_Sessions) {
        if (other == s) continue;
        uint8_t buf[3 + 4 + 1 + 2 + 2 + 1];
        st_PACKET_HEADER hdr;
        hdr.byCode = dfNETWORK_PACKET_CODE;
        hdr.bySize = sizeof(buf) - sizeof(st_PACKET_HEADER);
        hdr.byType = dfPACKET_SC_CREATE_OTHER_CHARACTER;
        memcpy(buf, &hdr, sizeof(hdr));
        uint32_t id_net = other->dwSessionID;
        memcpy(buf + 3, &id_net, 4);
        buf[7] = other->byDirection;
        uint16_t x_net = other->shX;
        uint16_t y_net = other->shY;
        memcpy(buf + 8, &x_net, 2);
        memcpy(buf + 10, &y_net, 2);
        buf[12] = (uint8_t)other->chHP;
        s->SendQ.Enqueue((char*)buf, (int)sizeof(buf));
    }

    //다른 클라이언트들에게 신규 접속자 정보 전송
    {
        unsigned char buf[3 + 4 + 1 + 2 + 2 + 1];
        st_PACKET_HEADER hdr;
        hdr.byCode = dfNETWORK_PACKET_CODE;
        hdr.bySize = sizeof(buf) - sizeof(st_PACKET_HEADER);
        hdr.byType = dfPACKET_SC_CREATE_OTHER_CHARACTER;
        memcpy(buf, &hdr, sizeof(hdr));
        unsigned int id_net = s->dwSessionID;
        memcpy(buf + 3, &id_net, 4);
        buf[7] = s->byDirection;
        unsigned short x_net = s->shX;
        unsigned short y_net = s->shY;
        memcpy(buf + 8, &x_net, 2);
        memcpy(buf + 10, &y_net, 2);
        buf[12] = (uint8_t)s->chHP;
        BroadcastPacketExcept(s, buf, (int)sizeof(buf));
    }
}

// recv
void netProc_Recv(st_SESSION* pSession) {
    if (!pSession) return;
    char tmp[4096];
    int ret = recv(pSession->Socket, tmp, (int)sizeof(tmp), 0);
    if (ret == 0) {
        cout << "Client closed (session " << pSession->dwSessionID << ")\n";
        Disconnect(pSession);
        return;
    }
    else if (ret == SOCKET_ERROR) {
        int e = WSAGetLastError();
        if (e == WSAEWOULDBLOCK) return;
        cerr << "recv error (" << pSession->dwSessionID << "): " << e << "\n";
        Disconnect(pSession);
        return;
    }
    else {
        // 링버퍼에 저장 (Enqueue)
        pSession->RecvQ.Enqueue(tmp, ret);
    }

    // 완성된 패킷이 있으면 처리
    while (pSession->RecvQ.GetUseSize() >= (int)sizeof(st_PACKET_HEADER)) {
        st_PACKET_HEADER hdr;
        int peeked = pSession->RecvQ.Peek((char*)&hdr, (int)sizeof(hdr));
        if (peeked < (int)sizeof(hdr)) break; // 안전
        if (hdr.byCode != dfNETWORK_PACKET_CODE) {
            cerr << "Bad packet code from session " << pSession->dwSessionID << "\n";
            Disconnect(pSession);
            return;
        }
        if (hdr.bySize < sizeof(st_PACKET_HEADER) || hdr.bySize > 255) {
            cerr << "Bad packet size from session " << pSession->dwSessionID << ": " << (int)hdr.bySize << "\n";
            Disconnect(pSession);
            return;
        }
        if (pSession->RecvQ.GetUseSize() < hdr.bySize) break; // 아직 전체 도착 안함

        // 전체 패킷을 꺼내서 처리 (Dequeue)
        vector<char> pkt(hdr.bySize);
        int got = pSession->RecvQ.Dequeue(pkt.data(), hdr.bySize + sizeof(st_PACKET_HEADER));
        if (got != hdr.bySize + sizeof(st_PACKET_HEADER)) {
            cerr << "Dequeue size mismatch\n";
            Disconnect(pSession);
            return;
        }
        char* payload = pkt.data() + sizeof(st_PACKET_HEADER);
        int payloadLen = hdr.bySize;
        if (!PacketProc(pSession, hdr.byType, payload, payloadLen)) {
            // 처리 실패 시 연결 종료
            Disconnect(pSession);
            return;
        }
    }
}

// send: SendQ -> 실제 send (Peek -> send -> Dequeue(sent))
void netProc_Send(st_SESSION* pSession) {
    if (!pSession) return;
    int avail = pSession->SendQ.GetUseSize();
    if (avail <= 0) return;

    int chunk = min(avail, 4096);
    vector<char> tmp(chunk);
    int peeked = pSession->SendQ.Peek(tmp.data(), chunk); // Peek: 제거하지 않음
    if (peeked <= 0) return;

    int sent = send(pSession->Socket, tmp.data(), peeked, 0);
    if (sent == SOCKET_ERROR) {
        int e = WSAGetLastError();
        if (e == WSAEWOULDBLOCK) {
            // 아무 것도 제거하지 않음 (데이터는 아직 SendQ에 있음)
            return;
        }
        else {
            cerr << "send error (" << pSession->dwSessionID << "): " << e << "\n";
            Disconnect(pSession);
            return;
        }
    }
    else if (sent > 0) {
        int dec = pSession->SendQ.Dequeue(nullptr, sent); 

        if (dec != sent) {
            vector<char> drop(sent);
            pSession->SendQ.Dequeue(drop.data(), sent);
        }
    }
}

// Disconnect
void Disconnect(st_SESSION* pSession) {
    if (!pSession) return;
    cout << "Disconnect session " << pSession->dwSessionID << "\n";

    // 다른 클라이언트에게 삭제 패킷 전송
    unsigned char buf[3 + 4];
    st_PACKET_HEADER hdr;
    hdr.byCode = dfNETWORK_PACKET_CODE;
    hdr.bySize = sizeof(buf) - sizeof(st_PACKET_HEADER);
    hdr.byType = dfPACKET_SC_DELETE_CHARACTER;
    memcpy(buf, &hdr, sizeof(hdr));
    unsigned int id_net = pSession->dwSessionID;
    memcpy(buf + 3, &id_net, 4);
    BroadcastPacketExcept(pSession, buf, (int)sizeof(buf));

    closesocket(pSession->Socket);
    auto it = find(g_Sessions.begin(), g_Sessions.end(), pSession);
    if (it != g_Sessions.end()) g_Sessions.erase(it);
    delete pSession;
}

// 브로드캐스트(발신자 제외)
void BroadcastPacketExcept(st_SESSION* exclude, const void* data, int len) {
    for (auto s : g_Sessions) {
        if (s == exclude) continue;
        s->SendQ.Enqueue((char*)data, len);
    }
}

// 특정 세션에 패킷 전송(큐에 저장)
void SendPacketToSession(st_SESSION* pSession, const void* data, int len) {
    if (!pSession) return;
    pSession->SendQ.Enqueue((char*)data, len);
}

// -------------------- 패킷 처리 --------------------
bool PacketProc(st_SESSION* pSession, uint8_t byPacketType, char* pPayload, int payloadLen) {
    switch (byPacketType) {
    case dfPACKET_CS_MOVE_START:
    {
        if (payloadLen < (int)sizeof(st_CS_MOVE_START)) return false;
        st_CS_MOVE_START cs;
        memcpy(&cs, pPayload, sizeof(cs));
        unsigned short rx = cs.shX;
        unsigned short ry = cs.shY;
        unsigned char dir = cs.byDirection;

        // 위치 오차 검사
        if ((int)abs((int)pSession->shX - (int)rx) > dfERROR_RANGE ||
            (int)abs((int)pSession->shY - (int)ry) > dfERROR_RANGE) {
            cerr << "Client out of sync, disconnecting session " << pSession->dwSessionID << "\n";
            return false;
        }

        // 상태 갱신
        pSession->dwAction = 1;
        pSession->byDirection = dir;
        pSession->shX = rx;
        pSession->shY = ry;
        cout << "# PACKET_MOVESTART # SessionID:" << pSession->dwSessionID << " / Direction:" << (int)pSession->byDirection << " / X:" << pSession->shX << " / Y:" << pSession->shY << "\n";

        unsigned char buf[3 + 4 + 1 + 2 + 2];
        st_PACKET_HEADER hdr;
        hdr.byCode = dfNETWORK_PACKET_CODE;
        hdr.bySize = sizeof(buf) - sizeof(st_PACKET_HEADER);
        hdr.byType = dfPACKET_SC_MOVE_START;
        memcpy(buf, &hdr, 3);
        unsigned int id_net = pSession->dwSessionID;
        memcpy(buf + 3, &id_net, 4);
        buf[7] = pSession->byDirection;
        unsigned short x_net = pSession->shX;
        unsigned short y_net = pSession->shY;
        memcpy(buf + 8, &x_net, 2);
        memcpy(buf + 10, &y_net, 2);
        BroadcastPacketExcept(pSession, buf, (int)sizeof(buf));
        return true;
    }

    case dfPACKET_CS_MOVE_STOP:
    {
        if (payloadLen < (int)sizeof(st_CS_MOVE_STOP)) return false;
        st_CS_MOVE_STOP cs;
        memcpy(&cs, pPayload, sizeof(cs));
        unsigned short rx = cs.shX;
        unsigned short ry = cs.shY;
        unsigned char dir = cs.byDirection;

        // 동기 검사
        if ((int)abs((int)pSession->shX - (int)rx) > dfERROR_RANGE ||
            (int)abs((int)pSession->shY - (int)ry) > dfERROR_RANGE) {
            cerr << "Client out of sync (stop), disconnect session " << pSession->dwSessionID << "\n";
            return false;
        }

        pSession->dwAction = 0;
        pSession->byDirection = dir;
        pSession->shX = rx;
        pSession->shY = ry;
        cout << "# PACKET_MOVESTOP # SessionID:" << pSession->dwSessionID << " / Direction:" << (int)pSession->byDirection << " / X:" << pSession->shX << " / Y:" << pSession->shY << "\n";

        // 브로드캐스트 SC_MOVE_STOP (hdr + ID(4) + dir(1) + X(2) + Y(2))
        unsigned char buf[3 + 4 + 1 + 2 + 2];
        st_PACKET_HEADER hdr;
        hdr.byCode = dfNETWORK_PACKET_CODE;
        hdr.bySize = sizeof(buf) - sizeof(st_PACKET_HEADER);
        hdr.byType = dfPACKET_SC_MOVE_STOP;
        memcpy(buf, &hdr, 3);
        unsigned int id_net = pSession->dwSessionID;
        memcpy(buf + 3, &id_net, 4);
        buf[7] = pSession->byDirection;
        unsigned short x_net = pSession->shX;
        unsigned short y_net = pSession->shY;
        memcpy(buf + 8, &x_net, 2);
        memcpy(buf + 10, &y_net, 2);
        BroadcastPacketExcept(pSession, buf, (int)sizeof(buf));
        return true;
    }

    case dfPACKET_CS_ATTACK1:
    case dfPACKET_CS_ATTACK2:
    case dfPACKET_CS_ATTACK3:
    {
        if (payloadLen < (int)sizeof(st_CS_ATTACK)) return false;
        st_CS_ATTACK cs;
        memcpy(&cs, pPayload, sizeof(cs));
        unsigned short rx = cs.shX;
        unsigned short ry = cs.shY;
        unsigned char dir = cs.byDirection;

        // 동기 검사
        if ((int)abs((int)pSession->shX - (int)rx) > dfERROR_RANGE ||
            (int)abs((int)pSession->shY - (int)ry) > dfERROR_RANGE) {
            cout<< "Client Session ID : "<< pSession->dwSessionID << " Client X: " << rx << " Y: " << ry << " Server X:" << pSession->shX << " Y:" << pSession->shY << "\n";
            cout << "Client out of sync (attack), disconnect session " << pSession->dwSessionID << "\n";
            return false;
        }

        // 위치/방향 업데이트
        pSession->byDirection = dir;
        pSession->shX = rx;
        pSession->shY = ry;

        // 공격 범위 내 충돌 검사 및 데미지 처리
        int ATTACK_RANGE_X = dfATTACK1_RANGE_X;
        int ATTACK_RANGE_Y = dfATTACK1_RANGE_Y;

        for (auto target : g_Sessions) {
            if (target == pSession) continue;
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

                unsigned char dmgBuf[3 + 4 + 4 + 1];
                st_PACKET_HEADER dmgHdr;
                dmgHdr.byCode = dfNETWORK_PACKET_CODE;
                dmgHdr.bySize = sizeof(dmgBuf) - sizeof(st_PACKET_HEADER);
                dmgHdr.byType = dfPACKET_SC_DAMAGE;
                memcpy(dmgBuf, &dmgHdr, 3);
                memcpy(dmgBuf + 3, &pSession->dwSessionID, 4);
                memcpy(dmgBuf + 7, &target->dwSessionID, 4);
                dmgBuf[11] = target->chHP;

                pSession->SendQ.Enqueue((char*)dmgBuf, sizeof(dmgBuf));
                BroadcastPacketExcept(pSession, dmgBuf, sizeof(dmgBuf));
                cout << "Damged Client Session ID : " << target->dwSessionID << " Client X: " << target->shX << " Y: " << target->shY << " HP:" << (int)target->chHP << "\n";
            }
        }

        // 공격 패킷 브로드캐스트
        unsigned char buf[3 + 4 + 1 + 2 + 2];
        st_PACKET_HEADER hdr;
        hdr.byCode = dfNETWORK_PACKET_CODE;
        hdr.bySize = sizeof(buf) - sizeof(st_PACKET_HEADER);
        unsigned char scType = (byPacketType == dfPACKET_CS_ATTACK1) ? dfPACKET_SC_ATTACK1 :
            (byPacketType == dfPACKET_CS_ATTACK2) ? dfPACKET_SC_ATTACK2 :
            dfPACKET_SC_ATTACK3;
        hdr.byType = scType;
        memcpy(buf, &hdr, 3);
        memcpy(buf + 3, &pSession->dwSessionID, 4);
        buf[7] = pSession->byDirection;
        memcpy(buf + 8, &pSession->shX, 2);
        memcpy(buf + 10, &pSession->shY, 2);
        BroadcastPacketExcept(pSession, buf, (int)sizeof(buf));
        cout << " # PACKET_ATTACK : " << "Attack Client Session ID : " << pSession->dwSessionID << " Client X: " << pSession->shX << " Y: " << pSession->shY << "\n";

        return true;
    }

    default:
        // 알려지지 않은 패킷 타입: 무시(또는 로그)
        cerr << "Unknown packet type from " << pSession->dwSessionID << ": " << (int)byPacketType << "\n";
        return true;
    }
    return true;
}

// -------------------- 게임 로직 --------------------
void UpdateLogic(double dt) {
    (void)dt;
    // 각 세션의 이동 처리: byDirection을 가지고 단순 이동 처리
    for (size_t idx = 0; idx < g_Sessions.size();) {
        st_SESSION* s = g_Sessions[idx];
        if (!s) { ++idx; continue; }

        if (s->chHP <= 0) {
            cout << "Dead Client Session ID : " << s->dwSessionID << " Client X: " << s->shX << " Y: " << s->shY << " HP:" << (int)s->chHP << "\n";
            Disconnect(s);
            continue;
        }

        if (s->dwAction == 1) {
            switch (s->byDirection) {
            case dfPACKET_MOVE_DIR_LL:
                if (s->shX > dfRANGE_MOVE_LEFT)
                {
                    s->shX = (unsigned short)max((int)dfRANGE_MOVE_LEFT, (int)s->shX - MOVE_UNIT_X);
                    cout << "# gameRun:RR # SessionID:" << s->dwSessionID << " / X:" << s->shX << " / Y:" << s->shY << "\n";
                }
                else s->byDirection = dfPACKET_MOVE_DIR_LL; // 멈춤 처리: 클라이언트 규격에 맞게 변경 가능
                break;
            case dfPACKET_MOVE_DIR_LU:
                if (s->shX > dfRANGE_MOVE_LEFT) {
                    s->shX = (unsigned short)max((int)dfRANGE_MOVE_LEFT, (int)s->shX - MOVE_UNIT_X);
                }
                if (s->shY > dfRANGE_MOVE_TOP) {
                    s->shY = (unsigned short)max((int)dfRANGE_MOVE_TOP, (int)s->shY - MOVE_UNIT_Y);
                }
                break;
            case dfPACKET_MOVE_DIR_UU:
                if (s->shY > dfRANGE_MOVE_TOP)
                {
                    s->shY = (unsigned short)max((int)dfRANGE_MOVE_TOP, (int)s->shY - MOVE_UNIT_Y);
                    cout << "# gameRun:RR # SessionID:" << s->dwSessionID << " / X:" << s->shX << " / Y:" << s->shY << "\n";
                }
                else s->byDirection = dfPACKET_MOVE_DIR_UU;
                break;
            case dfPACKET_MOVE_DIR_RU:
                if (s->shX > dfRANGE_MOVE_RIGHT) {
                    s->shX = (unsigned short)min((int)dfRANGE_MOVE_RIGHT, (int)s->shX + MOVE_UNIT_X);
                }
                if (s->shY > dfRANGE_MOVE_TOP) {
                    s->shY = (unsigned short)max((int)dfRANGE_MOVE_TOP, (int)s->shY - MOVE_UNIT_Y);
                }
                break;
            case dfPACKET_MOVE_DIR_RR:
                if (s->shX < dfRANGE_MOVE_RIGHT)
                {
                    s->shX = (unsigned short)min((int)dfRANGE_MOVE_RIGHT, (int)s->shX + MOVE_UNIT_X);
                    cout << "# gameRun:RR # SessionID:" << s->dwSessionID << " / X:" << s->shX << " / Y:" << s->shY << "\n";
                }
                else s->byDirection = dfPACKET_MOVE_DIR_RR;
                break;
            case dfPACKET_MOVE_DIR_RD:
                if (s->shX < dfRANGE_MOVE_RIGHT) {
                    s->shX = (unsigned short)min((int)dfRANGE_MOVE_RIGHT, (int)s->shX + MOVE_UNIT_X);
                }
                if (s->shY < dfRANGE_MOVE_BOTTOM) {
                    s->shY = (unsigned short)min((int)dfRANGE_MOVE_BOTTOM, (int)s->shY + MOVE_UNIT_Y);
                }
                break;
            
            case dfPACKET_MOVE_DIR_DD:
                if (s->shY < dfRANGE_MOVE_BOTTOM) {
                    cout << "# gameRun:RR # SessionID:" << s->dwSessionID << " / X:" << s->shX << " / Y:" << s->shY << "\n";
                    s->shY = (unsigned short)min((int)dfRANGE_MOVE_BOTTOM, (int)s->shY + MOVE_UNIT_Y);
                } 
                else s->byDirection = dfPACKET_MOVE_DIR_DD;
                break;
            case dfPACKET_MOVE_DIR_LD:
                if (s->shX > dfRANGE_MOVE_LEFT) {
                    s->shX = (unsigned short)max((int)dfRANGE_MOVE_LEFT, (int)s->shX - MOVE_UNIT_X);
                }
                if (s->shY < dfRANGE_MOVE_BOTTOM) {
                    s->shY = (unsigned short)min((int)dfRANGE_MOVE_BOTTOM, (int)s->shY + MOVE_UNIT_Y);
                }
                break;
            default:
                break;
            }
        }

        ++idx;
    }
}
