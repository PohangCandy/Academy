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
//#include <vector>
//#include <map>
#include <unordered_map>
#include <list>
#include <set>
#include <stack>
#include "cSessionMap.h"
#include "CPacket.h"
#include "Character.h"
using namespace std;



//1만여명이 싹다 Mss크기로 보낼 수 있는 경우 최대 버퍼 크기
//1600 * 10000
#define RINGBUFSIZE (1024 * 16)
#define BUFSIZE (1024 * 8)

// 전역
SOCKET g_ListenSocket = INVALID_SOCKET;
bool g_bShutdown = false;
uint32_t g_nextSessionID = 1;
cSessionMap* g_sessionMap = cSessionMap::GetSessionMap();

//------------------------------------------------------------- 
// 캐릭터 객체 메인 관리 
//------------------------------------------------------------- 
unordered_map<DWORD, c_CHARACTER*> g_CharacterMap;

//------------------------------------------------------------- 
// 월드맵 캐릭터 섹터 
//------------------------------------------------------------- 
unordered_map<int ,c_CHARACTER*> g_Sector[dfSECTOR_MAPMAX_Y + 1][dfSECTOR_MAPMAX_X + 1];

//------------------------------------------------------------- 
// 삭제한 캐릭터 ID
//------------------------------------------------------------- 
stack<int> g_DeleteCharacterID;
set<int> g_DeleteCheckSet;

//vector <SOCKETINFO* > g_Sessions;

void netIOProcess();
void netProc_Accept();
void netProc_Recv(SOCKETINFO*& pSession);
void netProc_Send(SOCKETINFO*& pSession);
//연결끊김 감지
//섹터 맵 삭제>캐릭터 맵 삭제 >캐릭터 삭제 >세션 맵 삭제 > 소켓 종료 > 세션 삭제
void Disconnect(SOCKETINFO*& pSession);
//소켓 닫기..이건 세션 맴버로 나중에 넣어야겠다.
void DisconnectSocket(SOCKETINFO*& pSession);
//캐릭터 맵, 캐릭터 삭제 + 세션 맵,세션 삭제
void DeleteDieCharacter();
//캐릭터 맵 삭제 + 캐릭터 delete
void DeleteCharacter(c_CHARACTER* pCharacter);
//---------------------------------------------------------------------------------------
// 섹터 맵에서 pop > 다른 캐릭터에게 삭제 패킷 전송 > 다른 캐릭터 삭제
//---------------------------------------------------------------------------------------
void DeleteCharacterFromSectorMap(c_CHARACTER* pCharacter, CPacket* pPacket, c_SECTOR_POS* pSector);


void SendPacket_Around(SOCKETINFO* psession, CPacket* pPacket, bool self, c_SECTOR_AROUND* pSectorRange);

bool PacketProc(SOCKETINFO* pSession, unsigned char byPacketType, CPacket*& Packet);
bool SendPacket(SOCKETINFO* pSession, unsigned char byPacketType);

bool netPacketProc_MoveStart(SOCKETINFO* pSession, CPacket* pPacket);
bool netPacketProc_MoveStop(SOCKETINFO* pSession, CPacket* pPacket);
bool netPacketProc_Attack1(SOCKETINFO* pSession, CPacket* pPacket);
bool netPacketProc_Attack2(SOCKETINFO* pSession, CPacket* pPacket);
bool netPacketProc_Attack3(SOCKETINFO* pSession, CPacket* pPacket);
bool netPacketProc_ECHO(SOCKETINFO* pSession, CPacket* pPacket);
bool netPacketProc_Damage(SOCKETINFO* pSession, CPacket* pPacket, int Attack_XRange, int Attack_YRange);
bool netPacketProc_Sync(SOCKETINFO* pSession, CPacket* pPacket);

bool clientPacketProc_CREATE_MY_CHARACTER(SOCKETINFO* pSession, CPacket* newPacket);

//신규 섹터 클라에게 기존의 다른 클라이언트 생성 패킷 전송
bool clientPacketProc_CREATE_OTHER_CHARACTER(SOCKETINFO* pSession, CPacket* newPacket);
//bool clientPacketProc_ECHO(SOCKETINFO* pSession, CPacket* newPacket);
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
void mpECHO(CPacket* pPacket, uint32_t Time);
void mpDisconnect(CPacket* pPacket, DWORD dwSessionID);

void Update();
void PrintPacket(const string& name, CPacket* pPacket);

//---------------------------------------------------------------------
// 플레이어의 섹터 업데이트 함수
// 플레이어가 원래 가지고 있던 섹터 정보를 기반으로
// 현재 좌표가 새로운 섹터라면 return true
//---------------------------------------------------------------------
bool IsSectorUpdate(c_CHARACTER* pCharacter);

//----------------------------------------------------------------------
// 섹터 맵에서 pop > 다른 캐릭터에게 삭제 패킷 전송 > 다른 캐릭터 삭제
// 섹터 맵 추가 > 다른 클라이언트의 캐릭터 생성 > 다른 클라이언트에게 클라이언트의 캐릭터 생성
//----------------------------------------------------------------------
void CharacterSectorUpdatePacket(c_CHARACTER* pCharacter,CPacket* pPacket);

//---------------------------------------------------------------------
//섹터 맵 추가 > 다른 클라이언트의 캐릭터 생성 > 다른 클라이언트에게 클라이언트의 캐릭터 생성
//---------------------------------------------------------------------
void AddCharacterToSectorMap(c_CHARACTER* pCharacter, CPacket* pPacket);



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
            //UpdateLogic(elapsed.count());
            Update();
            DeleteDieCharacter();
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
    quotientOfDivide64 = g_sessionMap->GetSize() / 64 + 1;

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
            }

            if (s && s->sock != INVALID_SOCKET) {
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


    SOCKETINFO* s = new SOCKETINFO(RINGBUFSIZE);
    long long id = g_sessionMap->AddSession(s);
    s->sock = clientSock;
    s->session_id = id;

    auto it = g_CharacterMap.find(id);

    if (it != g_CharacterMap.end()) {
        while (1)
        {
            cout << "duplicate character map\n";
        }
        g_CharacterMap.erase(it);
    }
   
    c_CHARACTER* character = new c_CHARACTER(s, id);
    g_CharacterMap.emplace(id, character);
    s->pCharacter = character;
   
    //cout << "Accepted new client (session " << s->session_id << ")\n";
    //신규 클라이언트에게 자기 캐릭터 할당 패킷 전송
    SendPacket(s, dfPACKET_SC_CREATE_MY_CHARACTER);

    //섹터 맵 추가 > 다른 클라이언트의 캐릭터 생성 > 다른 클라이언트에게 클라이언트의 캐릭터 생성
    CPacket* pPacket = new CPacket;
    AddCharacterToSectorMap(character,pPacket);
    delete pPacket;
}

// recv
void netProc_Recv(SOCKETINFO*& pSession) {
    if (!pSession) return;
    
    //받을게 있으면 지금 다 받게 해줄까?
    char tmp[BUFSIZE];
    int ret = recv(pSession->sock, tmp, BUFSIZE, 0);
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
        pSession->recvBuf->Enqueue(tmp,ret);
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

        if (!PacketProc(pSession, hdr.byType, pPacket)) {
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
void netProc_Send(SOCKETINFO*& pSession) {
    if (!pSession) return;
    int avail = pSession->sendBuf->GetUseSize();
    if (avail <= 0) return;

    if (avail < pSession->sendBuf->DirectDequeueSize())
    {
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
            int dec = pSession->sendBuf->MoveFront(sent);

            if (dec != sent) {
                while (1)
                {
                    cout << "[Send] 송신 버퍼에서 끄집어낸 크기가 달라짐\n";
                }
            }
        }
    }
    else
    {
        int directSize = pSession->sendBuf->DirectDequeueSize();
        int sent = send(pSession->sock, pSession->sendBuf->GetFrontBufferPtr(), directSize, 0);
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
            int dec = pSession->sendBuf->MoveFront(sent);

            if (dec != sent) {
                while (1)
                {
                    cout << "[Send] 송신 버퍼에서 끄집어낸 크기가 달라짐\n";
                }
            }
        }

        int frontSize = avail - directSize;
        sent = send(pSession->sock, pSession->sendBuf->GetBufPtr(), frontSize, 0);
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
            int dec = pSession->sendBuf->MoveFront(sent);

            if (dec != sent) {
                while (1)
                {
                    cout << "[Send] 송신 버퍼에서 끄집어낸 크기가 달라짐\n";
                }
            }
        }
    }
}

//섹터 맵 삭제>캐릭터 맵 삭제 >캐릭터 삭제 >세션 맵 삭제 > 소켓 종료 > 세션 삭제
void Disconnect(SOCKETINFO*& pSession) {
    if (!pSession) return;
    //cout << "Disconnect session " << pSession->session_id << "\n";
  

    //섹터 맵 삭제
    //현재 섹터에서 삭제
    CPacket* pPacket = new CPacket;
    DeleteCharacterFromSectorMap(pSession->pCharacter, pPacket, &pSession->pCharacter->CurSector);
    //캐릭터 맵 삭제 >캐릭터 삭제
    DeleteCharacter(pSession->pCharacter);

    //클라이언트 정보 얻기
    SOCKADDR_IN clientaddr;
    int addrlen = sizeof(clientaddr);
    getpeername(pSession->sock, (SOCKADDR*)&clientaddr, &addrlen);

    //세션 맵 삭제 > 소켓 종료 > 세션 삭제
    g_sessionMap->deleteSession(pSession, inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
    delete pPacket;
}

void DisconnectSocket(SOCKETINFO*& pSession)
{
    closesocket(pSession->sock);
    g_DeleteCharacterID.push(pSession->session_id);
    if (g_DeleteCheckSet.insert(pSession->session_id).second) {
    }
    else {
        while (1)
        {
            // 이미 삭제 대기열에 들어있다면 무시
            printf("[DEBUG] 이미 삭제 대기 중인 ID: %d\n", (int)pSession->session_id);
        }
    }
}

void DeleteDieCharacter()
{
    while (!g_DeleteCharacterID.empty())
    {
        int id = g_DeleteCharacterID.top();
        g_DeleteCharacterID.pop();
        g_DeleteCheckSet.erase(id);

        SOCKETINFO* s;
        g_sessionMap->GetSessionptr(id, s);

       
        //캐릭터 맵 > 캐릭터 삭제
        DeleteCharacter(s->pCharacter);
        
        //세션 맵, 세션 삭제
        if (s != nullptr)
        {
            //클라이언트 정보 얻기
            SOCKADDR_IN clientaddr;
            int addrlen = sizeof(clientaddr);
            getpeername(s->sock, (SOCKADDR*)&clientaddr, &addrlen);
            g_sessionMap->OnlydeleteSession(s, inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
        }
    }

}

void DeleteCharacter(c_CHARACTER* pCharacter)
{
    //캐릭터 맵 > 캐릭터 삭제
    auto it = g_CharacterMap.find(pCharacter->dwSessionID);
    if (it != g_CharacterMap.end())
    {
        c_CHARACTER* c = it->second;
        g_CharacterMap.erase(it);
        delete c;
    }
}

void DeleteCharacterFromSectorMap(c_CHARACTER* pCharacter, CPacket* pPacket, c_SECTOR_POS* pSector)
{
    if (pCharacter->CurSector.index == -1)
    {
        while (1)
        {
            printf("[DeleteCharacterFromSectorMap] 섹터 밖에 있는 캐릭터에 대해 처리중 말도 안됨\n");
        }
    }

   
    int sectorY = pSector->iY;
    int sectorX = pSector->iX;

    //섹터 맵에서 캐릭터 검색
    auto it = g_Sector[sectorY][sectorX].find(pCharacter->dwSessionID);
    if (it != g_Sector[sectorY][sectorX].end())
    {
        //섹터 맵 리스트에서 지우기
        g_Sector[sectorY][sectorX].erase(it);
        //printf("[DeleteCharacterAtSectorMap] session_id : %d SectorY :  %d SectorX : %d\n",
           // pCharacter->dwSessionID, pCharacter->CurSector.iY, pCharacter->CurSector.iX);

        mpDisconnect(pPacket, pCharacter->dwSessionID);

        //현재 섹터 기준으로 삭제 패킷 전송
        if (pSector == &pCharacter->CurSector)
        {
            SendPacket_Around(pCharacter->pSession, pPacket, false, &pCharacter->CurSectorRange);
            LOG_PACKET("DELETE_CHARACTER", pPacket);
        }
        //과거 섹터 기준으로 삭제 패킷 전송
        else if(pSector == &pCharacter->OldSector)
        {
            SendPacket_Around(pCharacter->pSession, pPacket, false, &pCharacter->OldSectorRange);
            LOG_PACKET("DELETE_CHARACTER", pPacket);

            //섹터 변경한 클라에게 과거 섹터 주변 클라이언트들 삭제 패킷 전송
            SOCKETINFO* session = pCharacter->pSession;
            for (int i = 0; i < 9; i++)
            {
                if (pCharacter->OldSectorRange.Around[i].index != -1)
                {
                    //섹터 리스트에 있는 모든 플레이어의 삭제 패킷 전송
                    for (const auto& pair : g_Sector[pCharacter->OldSectorRange.Around[i].iY][pCharacter->OldSectorRange.Around[i].iX]) {
                        c_CHARACTER* otherCharater = pair.second;
                        if (otherCharater == nullptr || otherCharater == pCharacter) continue;
                        mpDisconnect(pPacket, otherCharater->dwSessionID);
                        session->sendBuf->Enqueue(pPacket->GetBufferPtr(), pPacket->GetDataSize());
                    }

                }
            }
        }
        else
        {
            while (1)
            {
                printf("[DeleteCharacterFromSectorMap] 말도 안되는 섹터 타입을 보냄");
            }
        }
        
    }
}

//9개의 주변 섹터를 돌면서 모든 플레이어에게 전송
void SendPacket_Around(SOCKETINFO* psession, CPacket* pPacket, bool self, c_SECTOR_AROUND* pSectorRange)
{
    if (self)
    {
        for (int i = 0; i < 9; i++)
        {
            if (pSectorRange->Around[i].index != -1)
            {
                for (const auto& pair : g_Sector[pSectorRange->Around[i].iY][pSectorRange->Around[i].iX]) {
                    int sessionID = pair.first;
                    SOCKETINFO* s;
                    g_sessionMap->GetSessionptr(sessionID, s);
                    if (s == nullptr) continue;
                    s->sendBuf->Enqueue(pPacket->GetBufferPtr(), pPacket->GetDataSize());
                }
            }
        }
    }
    else
    {
        for (int i = 0; i < 9; i++)
        {
            if (pSectorRange->Around[i].index != -1)
            {
                for (const auto& pair : g_Sector[pSectorRange->Around[i].iY][pSectorRange->Around[i].iX]) {
                    int sessionID = pair.first;
                    SOCKETINFO* s;
                    g_sessionMap->GetSessionptr(sessionID, s);
                    if (s == nullptr || s == psession) continue;
                    s->sendBuf->Enqueue(pPacket->GetBufferPtr(), pPacket->GetDataSize());
                }
            }
        }
    }
}

//// 특정 세션에 패킷 전송(큐에 저장)
//void SendPacketToSession(SOCKETINFO* pSession, const void* data, int len) {
//    if (!pSession) return;
//    pSession->sendBuf->Enqueue((char*)data, len);
//}




bool CharacterMoveCheck(short NextshX, short NextshY)
{
    if (NextshX <= dfRANGE_MOVE_LEFT || NextshX >= dfRANGE_MOVE_RIGHT 
        || NextshY >= dfRANGE_MOVE_BOTTOM || NextshY <= dfRANGE_MOVE_TOP) return false;

    return true;
}

void Update(void)
{
    //… 적절한 게임 업데이트 처리 타이밍 계산 …  // 25fps 
        //return;
    // 게임 업데이트 처리를 CPU 100% 을 먹도록 해줄 순 없을 것입니다.  
    // 가장 좋은 것은 네트워크 송수신 스레드와, 로직 스레드를 분리하고  
   // 로직 스레드의 루프는 클라이언트 프레임에 대응 되도록 맞추는 것입니다. 
    // 지금 우리는 싱글 스레드로  네트워크 송수신 + 로직 루프 가 함께 처리 되어야 하는 상황이고 
   // Sleep 같은 대기를 통해 로직 프레임을 제어하면 네트워크 송수신의 응답성이 너무 떨어지게 
   // 됩니다. 그래서 기본 루프는 최대한 빠르게 돌리며 네트워크 처리를 시도하고, 로직 프레임은 
   // 클라이언트 프레임과 비슷하도록 로직을 돌릴 의도로 설계 하였습니다. 
    c_CHARACTER* pCharacter = NULL;
    unordered_map<DWORD, c_CHARACTER*>::iterator Iter;

    for (Iter = g_CharacterMap.begin(); Iter != g_CharacterMap.end(); )
    {
        pCharacter = Iter->second;
        Iter++;
        if (0 >= pCharacter->chHP)
        {
            // 사망처리. 
            //Disconnect(pCharacter->pSession);
            DisconnectSocket(pCharacter->pSession);
            CPacket* pPacket = new CPacket;
            DeleteCharacterFromSectorMap(pCharacter, pPacket, &pCharacter->CurSector);
            delete pPacket;
        }
        else
        {
            // 일정 시간동안 수신이 없으면 종료처리 
            //if (dwCurrentTick - pCharacter->pSession->dwLastRecvTime >
            //    dfNETWORK_PACKET_RECV_TIMEOUT)
            //{
            //    DisconnectSession(pCharacter->pSession->Socket);
            //    continue;
            //}

            //----------------------------------------------------------- 
            // 현재 동작에 따른 처리. 
            //----------------------------------------------------------- 
            switch (pCharacter->dwAction)
            {
            case dfPACKET_MOVE_DIR_LL:
                if (CharacterMoveCheck(pCharacter->shX - dfSPEED_PLAYER_X,
                    pCharacter->shY))
                {
                    pCharacter->shX -= dfSPEED_PLAYER_X;
                }
                break;

            case dfPACKET_MOVE_DIR_LU:
                if (CharacterMoveCheck(pCharacter->shX - dfSPEED_PLAYER_X,
                    pCharacter->shY - dfSPEED_PLAYER_Y))
                {
                    pCharacter->shX -= dfSPEED_PLAYER_X;
                    pCharacter->shY -= dfSPEED_PLAYER_Y;
                }
                break;

            case dfPACKET_MOVE_DIR_UU:
                if (CharacterMoveCheck(pCharacter->shX,
                    pCharacter->shY - dfSPEED_PLAYER_Y))
                    pCharacter->shY -= dfSPEED_PLAYER_Y;

                break;
            case dfPACKET_MOVE_DIR_RU:
                if (CharacterMoveCheck(pCharacter->shX + dfSPEED_PLAYER_X,
                    pCharacter->shY - dfSPEED_PLAYER_Y))
                {
                    pCharacter->shX += dfSPEED_PLAYER_X;
                    pCharacter->shY -= dfSPEED_PLAYER_Y;
                }
                break;

            case dfPACKET_MOVE_DIR_RR:
                if (CharacterMoveCheck(pCharacter->shX + dfSPEED_PLAYER_X, pCharacter->shY))
                {
                    pCharacter->shX += dfSPEED_PLAYER_X;
                }
                break;

            case dfPACKET_MOVE_DIR_RD:
                if (CharacterMoveCheck(pCharacter->shX + dfSPEED_PLAYER_X,
                    pCharacter->shY + dfSPEED_PLAYER_Y))
                {
                    pCharacter->shX += dfSPEED_PLAYER_X;
                    pCharacter->shY += dfSPEED_PLAYER_Y;
                }
                break;

            case dfPACKET_MOVE_DIR_DD:
                if (CharacterMoveCheck(pCharacter->shX, pCharacter->shY + dfSPEED_PLAYER_Y))
                {
                    pCharacter->shY += dfSPEED_PLAYER_Y;
                }
                break;

            case dfPACKET_MOVE_DIR_LD:
                if (CharacterMoveCheck(pCharacter->shX - dfSPEED_PLAYER_X,
                    pCharacter->shY + dfSPEED_PLAYER_Y))
                {
                    pCharacter->shX -= dfSPEED_PLAYER_X;
                    pCharacter->shY += dfSPEED_PLAYER_Y;
                }
                break;
            }
            if (pCharacter->dwAction >= dfPACKET_MOVE_DIR_LL && pCharacter->dwAction <= dfPACKET_MOVE_DIR_LD)
            {
                 //이동인 경우 섹터 업데이트를 함. 
                if (IsSectorUpdate(pCharacter))
                {
                    CPacket* pPacket = new CPacket;
                    

                    //printf("[UpdateSector] sessionid : %d  x : %d y : %d\n", pCharacter->dwSessionID, pCharacter->shX, pCharacter->shY);
                    //printf("[UpdateSectorRange]\n");
                    //for (int i = 0; i < 3; i++)
                    //{
                    //    for (int j = 0; j < 3; j++)
                    //    {
                    //        printf(" %d ", pCharacter->CurSectorRange.Around[i * 3 + j].index);
                    //    }
                    //    printf("\n");
                    //}
                    //printf("\n");

                    CharacterSectorUpdatePacket(pCharacter, pPacket);
                    delete pPacket;
                }
            }
        }
    }
}

void PrintPacket(const string& name, CPacket* pPacket)
{
    
    char log[100];
    memcpy(log, pPacket->GetBufferPtr(), pPacket->GetDataSize());
    log[pPacket->GetDataSize()] = '/0';
    printf("#Packet : ");
    printf("[ %s ]", name.c_str());
    for (int i = 0; i < pPacket->GetDataSize();i++)
    {
        printf("%02X", (unsigned char)log[i]);
    }
    cout << "\n";
    
    if (pPacket->GetDataSize() >= 20)
    {
        DebugBreak();
    }
}

bool IsSectorUpdate(c_CHARACTER* pCharacter)
{
    c_SECTOR_POS curSector = pCharacter->CurSector;
    int newIndex = pCharacter->GetUpdateCurSectorIndex();
    if (curSector.index != newIndex)
    {
        pCharacter->OldSector = curSector;
        pCharacter->UpdateCurSectorRange();
        //printf("[IsSectorUpdate] sessionId : %d , SectorIndex : %d\n", pCharacter->dwSessionID, pCharacter->CurSector.index);
        return true;
    }
    return false;
}

//지금 여기서 섹터 업데이트 순서를 확인해봐야 할 듯
void CharacterSectorUpdatePacket(c_CHARACTER* pCharacter, CPacket* pPacket)
{

    //과거 섹터에서 플레이어 삭제
    DeleteCharacterFromSectorMap(pCharacter, pPacket, &pCharacter->OldSector);
    //현재 섹터 맵에 추가하기
    AddCharacterToSectorMap(pCharacter, pPacket);
    
}

//섹터 맵 추가 > 다른 클라이언트의 캐릭터 생성 > 다른 클라이언트에게 클라이언트의 캐릭터 생성
void AddCharacterToSectorMap(c_CHARACTER* pCharacter, CPacket* pPacket)
{
    //섹터 맵 추가
    g_Sector[pCharacter->CurSector.iY][pCharacter->CurSector.iX].emplace(pCharacter->dwSessionID, pCharacter);
   // printf("[SetCharacterAtSectorMap] session_id : %d SectorY :  %d SectorX : %d  Index: %d\n",
       // pCharacter->dwSessionID, pCharacter->CurSector.iY, pCharacter->CurSector.iX, pCharacter->CurSector.index);

    //새로운 섹터의 다른 클라이언트 캐릭터 생성
    SOCKETINFO* session = pCharacter->pSession;
    clientPacketProc_CREATE_OTHER_CHARACTER(session, pPacket);
    
    //다른 클라이언트에게 섹터의 새로운 클라 캐릭터 생성
    mpCreateOtherCharater(pPacket, session->session_id,
        session->pCharacter->byDirection,
        session->pCharacter->shX,
        session->pCharacter->shY,
        session->pCharacter->chHP);
    SendPacket_Around(session, pPacket, false, &pCharacter->CurSectorRange);
    LOG_PACKET("mpCreateOtherCharater", pPacket);
}

//void DeleteCharacterFromOldSector(c_CHARACTER* pCharacter)
//{
//   auto it =  g_Sector[pCharacter->OldSector.iY][pCharacter->OldSector.iX].find(pCharacter->dwSessionID);
//   if (it != g_Sector[pCharacter->OldSector.iY][pCharacter->OldSector.iX].end())
//   {
//       g_Sector[pCharacter->OldSector.iY][pCharacter->OldSector.iX].erase(it);
//       //printf("[DeleteCharacterAtSectorMap] session_id : %d SectorY :  %d SectorX : %d\n",
//       //    pCharacter->dwSessionID, pCharacter->OldSector.iY, pCharacter->OldSector.iX);
//   }
//}

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
        return clientPacketProc_CREATE_OTHER_CHARACTER(pSession, newPacket);
        break;

    //case dfPACKET_SC_ECHO:
    //    hdr.bySize = sizeof(st_SC_ECHO);
    //    hdr.byType = dfPACKET_SC_ECHO;
    //    *newPacket << hdr.byCode;
    //    *newPacket << hdr.bySize;
    //    *newPacket << hdr.byType;
    //    return clientPacketProc_ECHO(pSession, newPacket);
    //    break;

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

    case dfPACKET_CS_ECHO:
        return netPacketProc_ECHO(pSession, pPacket);
    default:
        while (1)
        {
            //SOCKADDR_IN clientaddr;
            //int addrlen = sizeof(clientaddr);
            //getpeername(pSession->sock, (SOCKADDR*)&clientaddr, &addrlen);
            //char* s_ip = inet_ntoa(clientaddr.sin_addr);
            //int i_port = (int)ntohs(clientaddr.sin_port);
            //printf("[Network] 클라이언트: IP 주소 = %s, 포트번호 = %d\n", s_ip, i_port);
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
    if ((int)abs((int)pSession->pCharacter->shX - (int)shX) > dfERROR_RANGE ||
        (int)abs((int)pSession->pCharacter->shY - (int)shY) > dfERROR_RANGE) {
        netPacketProc_Sync(pSession, pPacket);
        LOG_PACKET("Sync", pPacket);
        shX = pSession->pCharacter->shX;
        shY = pSession->pCharacter->shY;
    }

    // 상태 갱신
    //----------------------------------------------------- 
    // 동작을 변경.  동작번호와, 방향값이 같다. 
    //----------------------------------------------------- 
    pSession->pCharacter->dwAction = byDirection;

    //----------------------------------------------------- 
    // 단순 방향표시용 byDirection (LL,RR) 과  
    // 이동시 8방향 (LL,LU,UU,RU,RR,RD,DD,LD) 용 MoveDirection 이 있음. 
    //----------------------------------------------------- 
    pSession->pCharacter->byMoveDirection = byDirection;

    //----------------------------------------------------- 
     // 방향을 변경. 
    //----------------------------------------------------- 
    switch (byDirection)
    {
    case dfPACKET_MOVE_DIR_RR:
    case dfPACKET_MOVE_DIR_RU:
    case dfPACKET_MOVE_DIR_RD:
        pSession->pCharacter->byDirection = dfPACKET_MOVE_DIR_RR;
        break;

    case dfPACKET_MOVE_DIR_LU:
    case dfPACKET_MOVE_DIR_LL:
    case dfPACKET_MOVE_DIR_LD:
        pSession->pCharacter->byDirection = dfPACKET_MOVE_DIR_LL;
        break;
    }
    pSession->pCharacter->shX = shX;
    pSession->pCharacter->shY = shY;
    
    //여기서 섹터 업데이트를 한다.. 왜지? 움직일때 오차범위를 벗어날 수 있다??
    //----------------------------------------------------------------------- 
    if (IsSectorUpdate(pSession->pCharacter))
    {
        //----------------------------------------------------------------------- 
        // 섹터가 변경된 경우는 클라에게 관련 정보를 쏜다.
        //----------------------------------------------------------------------- 
        CharacterSectorUpdatePacket(pSession->pCharacter, pPacket);
    }
    // ----------------------------------------------------------------------- 

    mpMoveStart(pPacket, pSession->session_id, byDirection, pSession->pCharacter->shX, pSession->pCharacter->shY);


    //----------------------------------------------------- 
    // 현재 접속중인 사용자에게 모든 패킷을 뿌린다. (섹터 단위 패킷 전송 함수 ) 
    //----------------------------------------------------- 
    SendPacket_Around(pSession, pPacket, true, &pSession->pCharacter->CurSectorRange);
    LOG_PACKET("MoveStart", pPacket);

    return true;
}

bool netPacketProc_MoveStop(SOCKETINFO* pSession, CPacket* pPacket)
{
    if (pPacket->GetDataSize() < (int)sizeof(st_CS_MOVE_STOP)) return false;

    BYTE byDirection;
    short shX, shY;

    *pPacket >> byDirection;
    *pPacket >> shX;
    *pPacket >> shY;

    //_LOG(dfLOG_LEVEL_DEBUG, L"# MOVESTART # SessionID:%d / Direction:%d / X:%d / Y:%d",
   //    pSession->dwSessionID, byDirection, shX, shY);

   // 위치 오차 검사
    if ((int)abs((int)pSession->pCharacter->shX - (int)shX) > dfERROR_RANGE ||
        (int)abs((int)pSession->pCharacter->shY - (int)shY) > dfERROR_RANGE) {
        netPacketProc_Sync(pSession, pPacket);
        LOG_PACKET("Sync", pPacket);
        shX = pSession->pCharacter->shX;
        shY = pSession->pCharacter->shY;
    }

    // 상태 갱신
    //----------------------------------------------------- 
    // 동작을 변경.  동작번호와, 방향값이 같다. 
    //----------------------------------------------------- 
    pSession->pCharacter->dwAction = dfPACKET_CS_MOVE_STOP;

    //----------------------------------------------------- 
    // 단순 방향표시용 byDirection (LL,RR) 과  
    // 이동시 8방향 (LL,LU,UU,RU,RR,RD,DD,LD) 용 MoveDirection 이 있음. 
    //----------------------------------------------------- 
    pSession->pCharacter->byMoveDirection = byDirection;

    //----------------------------------------------------- 
     // 방향을 변경. 
    //----------------------------------------------------- 
    switch (byDirection)
    {
    case dfPACKET_MOVE_DIR_RR:
    case dfPACKET_MOVE_DIR_RU:
    case dfPACKET_MOVE_DIR_RD:
        pSession->pCharacter->byDirection = dfPACKET_MOVE_DIR_RR;
        break;

    case dfPACKET_MOVE_DIR_LU:
    case dfPACKET_MOVE_DIR_LL:
    case dfPACKET_MOVE_DIR_LD:
        pSession->pCharacter->byDirection = dfPACKET_MOVE_DIR_LL;
        break;
    }
    pSession->pCharacter->shX = shX;
    pSession->pCharacter->shY = shY;

    // 정지를 하면서 좌표가 약간 조절된 경우 섹터 업데이트
    //----------------------------------------------------------------------- 
    if (IsSectorUpdate(pSession->pCharacter))
    {
        //----------------------------------------------------------------------- 
        // 섹터가 변경된 경우는 클라에게 관련 정보를 쏜다.
        //----------------------------------------------------------------------- 
        CharacterSectorUpdatePacket(pSession->pCharacter, pPacket);
    }
   // ----------------------------------------------------------------------- 

    mpMoveStop(pPacket, pSession->session_id, byDirection, pSession->pCharacter->shX, pSession->pCharacter->shY);


    //----------------------------------------------------- 
    // 현재 접속중인 사용자에게 모든 패킷을 뿌린다. (섹터 단위 패킷 전송 함수 ) 
    //----------------------------------------------------- 
    SendPacket_Around(pSession, pPacket, true, &pSession->pCharacter->CurSectorRange);
    LOG_PACKET("MoveStop", pPacket);
    return true;
}

bool netPacketProc_Attack1(SOCKETINFO* pSession, CPacket* pPacket)
{
    if (pPacket->GetDataSize() < (int)sizeof(st_CS_ATTACK)) return false;

    BYTE byDirection;
    short shX, shY;

    *pPacket >> byDirection;
    *pPacket >> shX;
    *pPacket >> shY;

    //_LOG(dfLOG_LEVEL_DEBUG, L"# MOVESTART # SessionID:%d / Direction:%d / X:%d / Y:%d",
   //    pSession->dwSessionID, byDirection, shX, shY);

   // 위치 오차 검사
    if ((int)abs((int)pSession->pCharacter->shX - (int)shX) > dfERROR_RANGE ||
        (int)abs((int)pSession->pCharacter->shY - (int)shY) > dfERROR_RANGE) {
        netPacketProc_Sync(pSession, pPacket);
        LOG_PACKET("Sync", pPacket);
        shX = pSession->pCharacter->shX;
        shY = pSession->pCharacter->shY;
    }

    // 상태 갱신
     //----------------------------------------------------- 
     // 동작을 변경.  동작번호와, 방향값이 같다.
     //----------------------------------------------------- 
     pSession->pCharacter->dwAction = dfPACKET_CS_ATTACK1;

    //----------------------------------------------------- 
    // 단순 방향표시용 byDirection (LL,RR) 과  
    // 이동시 8방향 (LL,LU,UU,RU,RR,RD,DD,LD) 용 MoveDirection 이 있음. 
    //----------------------------------------------------- 
    pSession->pCharacter->byMoveDirection = byDirection;

    //----------------------------------------------------- 
     // 방향을 변경. 
    //----------------------------------------------------- 
    switch (byDirection)
    {
    case dfPACKET_MOVE_DIR_RR:
    case dfPACKET_MOVE_DIR_RU:
    case dfPACKET_MOVE_DIR_RD:
        pSession->pCharacter->byDirection = dfPACKET_MOVE_DIR_RR;
        break;

    case dfPACKET_MOVE_DIR_LU:
    case dfPACKET_MOVE_DIR_LL:
    case dfPACKET_MOVE_DIR_LD:
        pSession->pCharacter->byDirection = dfPACKET_MOVE_DIR_LL;
        break;
    }
    pSession->pCharacter->shX = shX;
    pSession->pCharacter->shY = shY;

    if (IsSectorUpdate(pSession->pCharacter))
    {
        //----------------------------------------------------------------------- 
        // 섹터가 변경된 경우는 클라에게 관련 정보를 쏜다.
        //----------------------------------------------------------------------- 
        CharacterSectorUpdatePacket(pSession->pCharacter, pPacket);
    }

    // 공격 범위 내 충돌 검사 및 데미지 처리
    int ATTACK_RANGE_X = dfATTACK1_RANGE_X;
    int ATTACK_RANGE_Y = dfATTACK1_RANGE_Y;

    netPacketProc_Damage(pSession, pPacket, dfATTACK1_RANGE_X, dfATTACK1_RANGE_Y);

    mpAttack(pPacket, pSession->session_id, pSession->pCharacter->byDirection,
        pSession->pCharacter->shX, pSession->pCharacter->shY, dfPACKET_SC_ATTACK1);
    SendPacket_Around(pSession, pPacket, true, &pSession->pCharacter->CurSectorRange);
    LOG_PACKET("Attack1", pPacket);
    //cout << " # PACKET_ATTACK : " << "Attack Client Session ID : " << pSession->session_id << " Client X: " << pSession->shX << " Y: " << pSession->shY << "\n";

    return true;
}

bool netPacketProc_Attack2(SOCKETINFO* pSession, CPacket* pPacket)
{
    if (pPacket->GetDataSize() < (int)sizeof(st_CS_ATTACK)) return false;

    BYTE byDirection;
    short shX, shY;

    *pPacket >> byDirection;
    *pPacket >> shX;
    *pPacket >> shY;

    //_LOG(dfLOG_LEVEL_DEBUG, L"# MOVESTART # SessionID:%d / Direction:%d / X:%d / Y:%d",
   //    pSession->dwSessionID, byDirection, shX, shY);

   // 위치 오차 검사
    if ((int)abs((int)pSession->pCharacter->shX - (int)shX) > dfERROR_RANGE ||
        (int)abs((int)pSession->pCharacter->shY - (int)shY) > dfERROR_RANGE) {
        netPacketProc_Sync(pSession, pPacket);
        LOG_PACKET("Sync", pPacket);
        shX = pSession->pCharacter->shX;
        shY = pSession->pCharacter->shY;
    }

    // 상태 갱신
     //----------------------------------------------------- 
     // 동작을 변경.  동작번호와, 방향값이 같다. ??
     //----------------------------------------------------- 
    pSession->pCharacter->dwAction = dfPACKET_CS_ATTACK2;

    //----------------------------------------------------- 
    // 단순 방향표시용 byDirection (LL,RR) 과  
    // 이동시 8방향 (LL,LU,UU,RU,RR,RD,DD,LD) 용 MoveDirection 이 있음. 
    //----------------------------------------------------- 
    pSession->pCharacter->byMoveDirection = byDirection;

    //----------------------------------------------------- 
     // 방향을 변경. 
    //----------------------------------------------------- 
    switch (byDirection)
    {
    case dfPACKET_MOVE_DIR_RR:
    case dfPACKET_MOVE_DIR_RU:
    case dfPACKET_MOVE_DIR_RD:
        pSession->pCharacter->byDirection = dfPACKET_MOVE_DIR_RR;
        break;

    case dfPACKET_MOVE_DIR_LU:
    case dfPACKET_MOVE_DIR_LL:
    case dfPACKET_MOVE_DIR_LD:
        pSession->pCharacter->byDirection = dfPACKET_MOVE_DIR_LL;
        break;
    }
    pSession->pCharacter->shX = shX;
    pSession->pCharacter->shY = shY;

    if (IsSectorUpdate(pSession->pCharacter))
    {
        //----------------------------------------------------------------------- 
        // 섹터가 변경된 경우는 클라에게 관련 정보를 쏜다.
        //----------------------------------------------------------------------- 
        CharacterSectorUpdatePacket(pSession->pCharacter, pPacket);
    }

    // 공격 범위 내 충돌 검사 및 데미지 처리
    int ATTACK_RANGE_X = dfATTACK2_RANGE_X;
    int ATTACK_RANGE_Y = dfATTACK2_RANGE_Y;

    netPacketProc_Damage(pSession, pPacket, dfATTACK2_RANGE_X, dfATTACK2_RANGE_Y);

    pPacket->Clear();
    mpAttack(pPacket, pSession->session_id, pSession->pCharacter->byDirection,
        pSession->pCharacter->shX, pSession->pCharacter->shY, dfPACKET_SC_ATTACK2);
    SendPacket_Around(pSession, pPacket, true, &pSession->pCharacter->CurSectorRange);
    LOG_PACKET("Attack2",pPacket);

    //cout << " # PACKET_ATTACK : " << "Attack Client Session ID : " << pSession->session_id << " Client X: " << pSession->shX << " Y: " << pSession->shY << "\n";

    return true;
}

bool netPacketProc_Attack3(SOCKETINFO* pSession, CPacket* pPacket)
{
    if (pPacket->GetDataSize() < (int)sizeof(st_CS_ATTACK)) return false;

    BYTE byDirection;
    short shX, shY;

    *pPacket >> byDirection;
    *pPacket >> shX;
    *pPacket >> shY;

    //_LOG(dfLOG_LEVEL_DEBUG, L"# MOVESTART # SessionID:%d / Direction:%d / X:%d / Y:%d",
   //    pSession->dwSessionID, byDirection, shX, shY);

   // 위치 오차 검사
    if ((int)abs((int)pSession->pCharacter->shX - (int)shX) > dfERROR_RANGE ||
        (int)abs((int)pSession->pCharacter->shY - (int)shY) > dfERROR_RANGE) {

        netPacketProc_Sync(pSession, pPacket);
        LOG_PACKET("Sync",pPacket);
        shX = pSession->pCharacter->shX;
        shY = pSession->pCharacter->shY;
    }

    // 상태 갱신
     //----------------------------------------------------- 
     // 동작을 변경.  동작번호와, 방향값이 같다.
     //----------------------------------------------------- 
    pSession->pCharacter->dwAction = dfPACKET_CS_ATTACK3;

    //----------------------------------------------------- 
    // 단순 방향표시용 byDirection (LL,RR) 과  
    // 이동시 8방향 (LL,LU,UU,RU,RR,RD,DD,LD) 용 MoveDirection 이 있음. 
    //----------------------------------------------------- 
    pSession->pCharacter->byMoveDirection = byDirection;

    //----------------------------------------------------- 
     // 방향을 변경. 
    //----------------------------------------------------- 
    switch (byDirection)
    {
    case dfPACKET_MOVE_DIR_RR:
    case dfPACKET_MOVE_DIR_RU:
    case dfPACKET_MOVE_DIR_RD:
        pSession->pCharacter->byDirection = dfPACKET_MOVE_DIR_RR;
        break;

    case dfPACKET_MOVE_DIR_LU:
    case dfPACKET_MOVE_DIR_LL:
    case dfPACKET_MOVE_DIR_LD:
        pSession->pCharacter->byDirection = dfPACKET_MOVE_DIR_LL;
        break;
    }
    pSession->pCharacter->shX = shX;
    pSession->pCharacter->shY = shY;

    if (IsSectorUpdate(pSession->pCharacter))
    {
        //----------------------------------------------------------------------- 
        // 섹터가 변경된 경우는 클라에게 관련 정보를 쏜다.
        //----------------------------------------------------------------------- 
        CharacterSectorUpdatePacket(pSession->pCharacter, pPacket);
    }

    // 공격 범위 내 충돌 검사 및 데미지 처리
    int ATTACK_RANGE_X = dfATTACK3_RANGE_X;
    int ATTACK_RANGE_Y = dfATTACK3_RANGE_Y;

    netPacketProc_Damage(pSession, pPacket, dfATTACK3_RANGE_X, dfATTACK3_RANGE_Y);

    pPacket->Clear();
    mpAttack(pPacket, pSession->session_id, pSession->pCharacter->byDirection,
    pSession->pCharacter->shX, pSession->pCharacter->shY, dfPACKET_SC_ATTACK3);
    SendPacket_Around(pSession, pPacket, true, &pSession->pCharacter->CurSectorRange);
    LOG_PACKET("Attack3",pPacket);
    //cout << " # PACKET_ATTACK : " << "Attack Client Session ID : " << pSession->session_id << " Client X: " << pSession->shX << " Y: " << pSession->shY << "\n";

    return true;
}

bool netPacketProc_ECHO(SOCKETINFO* pSession, CPacket* pPacket)
{

    if (pPacket->GetDataSize() < (int)sizeof(st_CS_ECHO)) return false;

    uint32_t Time;

    *pPacket >> Time;

    pPacket->Clear();
    mpECHO(pPacket, Time);
    pSession->sendBuf->Enqueue(pPacket->GetBufferPtr(), pPacket->GetDataSize());
    return true;
}

bool netPacketProc_Damage(SOCKETINFO* pSession, CPacket* pPacket, int Attack_XRange, int Attack_YRange)
{
    for (int i = 0; i < 9; i++)
    {
        if (pSession->pCharacter->CurSectorRange.Around[i].index != -1)
        {
            for (const auto& pair : g_Sector[pSession->pCharacter->CurSectorRange.Around[i].iY][pSession->pCharacter->CurSectorRange.Around[i].iX]) {
                int sessionID = pair.first;
                SOCKETINFO* target;
                g_sessionMap->GetSessionptr(sessionID, target);
                if (target == nullptr || target == pSession) continue;
                if (target->pCharacter->chHP <= 0) continue;

                bool rangeDirection = false;
                if (((int)target->pCharacter->shX >= (int)pSession->pCharacter->shX &&
                    pSession->pCharacter->byDirection == dfPACKET_MOVE_DIR_RR) ||
                    ((int)target->pCharacter->shX <= (int)pSession->pCharacter->shX &&
                        pSession->pCharacter->byDirection == dfPACKET_MOVE_DIR_LL))
                {
                    rangeDirection = true;
                }

                unsigned int dx = abs((int)target->pCharacter->shX - (int)pSession->pCharacter->shX);
                unsigned int dy = abs((int)target->pCharacter->shY - (int)pSession->pCharacter->shY);

                if (dx <= Attack_XRange && rangeDirection && dy <= Attack_YRange) {
                    target->pCharacter->chHP -= 10;

                    pPacket->Clear();
                    mpDamage(pPacket, pSession->session_id, target->session_id, target->pCharacter->chHP);
                    SendPacket_Around(target, pPacket, true, &pSession->pCharacter->CurSectorRange);
                    LOG_PACKET("Damage", pPacket);
                    //cout << "Damged Client Session ID : " << target->session_id << " Client X: " << target->shX << " Y: " << target->shY << " HP:" << (int)target->chHP << "\n";
                }
            }
        }
    }
    return false;
}

bool netPacketProc_Sync(SOCKETINFO* pSession, CPacket* pPacket)
{
    mpSync(pPacket, pSession->session_id, pSession->pCharacter->shX, pSession->pCharacter->shY);
    pSession->sendBuf->Enqueue(pPacket->GetBufferPtr(), pPacket->GetDataSize());
    return false;
}



bool clientPacketProc_CREATE_MY_CHARACTER(SOCKETINFO* pSession, CPacket* newPacket)
{
    *newPacket << (long)pSession->session_id;
    *newPacket << pSession->pCharacter->byDirection;
    *newPacket << pSession->pCharacter->shX;
    *newPacket << pSession->pCharacter->shY;
    *newPacket << pSession->pCharacter->chHP;

    LOG_PACKET("CreateMyCharacter", newPacket);
    //cout << "Create Client Session ID : " << pSession->session_id << " Client X: " << pSession->shX << " Y: " << pSession->shY << "\n";
    int enqdata = pSession->sendBuf->Enqueue(newPacket->GetBufferPtr(), newPacket->GetDataSize());
    if (enqdata != newPacket->GetDataSize())
    {
        while (1)
        {
            printf("[SendPacket] 송신 버퍼에 넣기 실패\n");
        }
    }

    return true;
}

bool clientPacketProc_CREATE_OTHER_CHARACTER(SOCKETINFO* pSession, CPacket* newPacket)
{
    st_PACKET_HEADER hdr;
    hdr.byCode = dfPACKET_CODE;
    hdr.bySize = sizeof(st_SC_CREATE_OTHER_CHARACTER);
    hdr.byType = dfPACKET_SC_CREATE_OTHER_CHARACTER;

    c_CHARACTER* c = pSession->pCharacter;
    for (int i = 0; i < 9; i++)
    {
        if (c->CurSectorRange.Around[i].index != -1)
        {
            for (const auto& pair : g_Sector[c->CurSectorRange.Around[i].iY][c->CurSectorRange.Around[i].iX]) {
                int sessionID = pair.first;
                SOCKETINFO* other;
                g_sessionMap->GetSessionptr(sessionID, other);
                if (other == nullptr || other == pSession) continue;

                newPacket->Clear();
                *newPacket << hdr.byCode;
                *newPacket << hdr.bySize;
                *newPacket << hdr.byType;

                *newPacket << (long)other->session_id;
                *newPacket << other->pCharacter->byDirection;
                *newPacket << other->pCharacter->shX;
                *newPacket << other->pCharacter->shY;
                *newPacket << other->pCharacter->chHP;

                LOG_PACKET("CreateOtherCharacter", newPacket);
                int enqdata = pSession->sendBuf->Enqueue(newPacket->GetBufferPtr(), newPacket->GetDataSize());
                if (enqdata != newPacket->GetDataSize())
                {
                    while (1)
                    {
                        printf("[SendPacket] 송신 버퍼에 넣기 실패");
                    }
                }
            }
        }
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
    stPacketHeader.bySize = sizeof(st_SC_CREATE_OTHER_CHARACTER);
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
    stPacketHeader.bySize = sizeof(st_SC_MOVE_START);
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
    stPacketHeader.bySize = sizeof(st_SC_MOVE_STOP);
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

void mpECHO(CPacket* pPacket, uint32_t Time)
{
    st_PACKET_HEADER stPacketHeader;
    stPacketHeader.byCode = dfPACKET_CODE;
    stPacketHeader.bySize = sizeof(st_SC_ECHO);
    stPacketHeader.byType = dfPACKET_SC_ECHO;
    pPacket->Clear();
    pPacket->PutData((char*)&stPacketHeader, sizeof(st_PACKET_HEADER));
    *pPacket << (long)Time;
}

void mpDisconnect(CPacket* pPacket, DWORD dwSessionID)
{
    st_PACKET_HEADER stPacketHeader;
    stPacketHeader.byCode = dfPACKET_CODE;
    stPacketHeader.bySize = sizeof(st_SC_DELETE_CHARACTER);
    stPacketHeader.byType = dfPACKET_SC_DELETE_CHARACTER;
    pPacket->Clear();
    pPacket->PutData((char*)&stPacketHeader, sizeof(st_PACKET_HEADER));
    *pPacket << (long)dwSessionID;
}
