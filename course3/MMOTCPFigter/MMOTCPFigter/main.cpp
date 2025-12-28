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
#include "MemoryPool.h"
#include "Dump.h"
#include "Profile.h"
using namespace std;




#define BUFSIZE (1024 * 8)
#define MAX_CLIENTS (13000)

// 전역
SOCKET g_ListenSocket = INVALID_SOCKET;
bool g_bShutdown = false;
cSessionMap* g_sessionMap = cSessionMap::GetSessionMap();

//-------------------------------------------
// 패킷 메모리 풀
//-------------------------------------------
procademy::CMemoryPool<CPacket> CPacketPool(1000, true);
procademy::CMemoryPool<c_CHARACTER> ChacracterPool(20000, true);

//------------------------------------------------------------- 
// 월드맵 캐릭터 섹터 
//------------------------------------------------------------- 
unordered_map<int ,c_CHARACTER*> g_Sector[dfSECTOR_MAPMAX_Y + 1][dfSECTOR_MAPMAX_X + 1];

//------------------------------------------------------------- 
// HP가 0이 된 캐릭터 ID
//------------------------------------------------------------- 
stack<c_CHARACTER*> g_DieCharacterID;

//--------------------------------------------------------
//프로파일링용 함수 이름
//--------------------------------------------------------
WCHAR f1[] = L"IOFunc";
WCHAR f2[] = L"DeleteLogic";
WCHAR f3[] = L"GameLogic";

//vector <SOCKETINFO* > g_Sessions;
SOCKETINFO* g_SessionArray[MAX_CLIENTS];
int g_CurSessionArraySize = 0;
stack<int> g_DeletedSessionArrayIndex;


void netIOProcess();
void netProc_Accept();
void netProc_Recv(SOCKETINFO*& pSession);
void netProc_Send(SOCKETINFO*& pSession);
//연결끊김 감지 > 캐릭터와 세션을 삭제 예정 리스트에 추가
void Disconnect(SOCKETINFO* pSession);
//캐릭터 맵, 캐릭터 삭제 + 세션 맵,세션 삭제
void DeleteDieCharacter();



void SendPacket_Around(SOCKETINFO* psession, CPacket* pPacket, bool self, c_SECTOR_AROUND* pSectorRange);

// 특정 섹터 1개에 있는 클라이언트들 에게 메시지 보내기
void SendPacket_SectorOne(c_SECTOR_POS* pSector, CPacket* pPacket, SOCKETINFO* pExceptSession);
// 특정 1명의 클라이언트에게 특정 섹터 1개에 있는 클라이언트들 삭제 메시지 보내기 
void SendOtherCharaterDeletePacket_Unicast(SOCKETINFO* pSession, CPacket* pPacket, c_SECTOR_POS* pSector);
// 특정 1명의 클라이언트에게 특정 섹터 1개에 있는 클라이언트들 생성 메시지 보내기 
void SendCreateCharaterPacket_Unicast(SOCKETINFO* pSession, CPacket* pPacket, c_SECTOR_POS* pSector);
// 특정 1명의 클라이언트 에게 메시지 보내기 
void SendPacket_Unicast(SOCKETINFO* pSession, CPacket* pPacket);

bool PacketProc(SOCKETINFO* pSession, unsigned char byPacketType, CPacket*& Packet);

bool netPacketProc_MoveStart(SOCKETINFO* pSession, CPacket* pPacket);
bool netPacketProc_MoveStop(SOCKETINFO* pSession, CPacket* pPacket);
bool netPacketProc_Attack1(SOCKETINFO* pSession, CPacket* pPacket);
bool netPacketProc_Attack2(SOCKETINFO* pSession, CPacket* pPacket);
bool netPacketProc_Attack3(SOCKETINFO* pSession, CPacket* pPacket);
bool netPacketProc_ECHO(SOCKETINFO* pSession, CPacket* pPacket);
bool netPacketProc_Damage(SOCKETINFO* pSession, CPacket* pPacket, int Attack_XRange, int Attack_YRange, BYTE Damage);
bool netPacketProc_Sync(SOCKETINFO* pSession, CPacket* pPacket);

bool clientPacketProc_CREATE_MY_CHARACTER(SOCKETINFO* pSession, CPacket* pPacket);

//신규 섹터 클라에게 기존의 다른 클라이언트 생성 패킷 전송
bool clientPacketProc_CREATE_OTHER_CHARACTER(SOCKETINFO* pSession, CPacket* pPacket);

void mpCreateCharaterToOtherAndGiveMovePacket(CPacket* pPacket, DWORD dwSessionID,BYTE byDir, BYTE moveDir, short shX, short shY, BYTE hp);
void mpCreateCharaterToOther(CPacket* pPacket, DWORD dwSessionID, BYTE byDir, short shX, short shY, BYTE hp);
void mpDamage(CPacket* pPacket, DWORD dwAttackerSessionID, DWORD dwDamagerID, BYTE hp);
void mpAttack(CPacket* pPacket, DWORD dwSessionID, BYTE byDir, short shX, short shY, char attackType);
void mpMoveStart(CPacket* pPacket, DWORD dwSessionID, BYTE byDir, short shX, short shY);
void mpMoveStop(CPacket* pPacket, DWORD dwSessionID, BYTE byDir, short shX, short shY);
void mpSync(CPacket* pPacket, DWORD dwSessionID, short shX, short shY);
void mpECHO(CPacket* pPacket, uint32_t Time);
void mpDelete(CPacket* pPacket, DWORD dwSessionID);

void Update(int frame);
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
void CharacterSectorUpdatePacket(c_CHARACTER* pCharacter);

//---------------------------------------------------------------------
//섹터 맵 추가 > 다른 클라이언트의 캐릭터 생성 > 다른 클라이언트에게 클라이언트의 캐릭터 생성
//---------------------------------------------------------------------
void AddCharacterToSectorMap(c_CHARACTER* pCharacter, CPacket* pPacket);
//---------------------------------------------------------------------------------------
// 섹터 맵에서 pop > 다른 캐릭터에게 삭제 패킷 전송 > 다른 캐릭터 삭제
//---------------------------------------------------------------------------------------
void DeleteCharacterFromSectorMap(c_CHARACTER* pCharacter, CPacket* pPacket, c_SECTOR_POS* pSector);



int main() {
    SetPriorityClass(GetCurrentProcess(), ABOVE_NORMAL_PRIORITY_CLASS);
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
    SetThreadIdealProcessor(GetCurrentThread(), 0);

    //덤프를 위한 작업
    SetUnhandledExceptionFilter(MyUnhandledExceptionFilter);

    timeBeginPeriod(1);

    for (int i = 0; i < dfSECTOR_MAPMAX_Y + 1; i++)
    {
        for (int j = 0; j < dfSECTOR_MAPMAX_X + 1; j++)
        {
            g_Sector[i][j].reserve(15000);
        }
    }

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

    if (listen(g_ListenSocket, SOMAXCONN_HINT(65535)) == SOCKET_ERROR) {
        cerr << "listen failed\n";
        closesocket(g_ListenSocket);
        WSACleanup();
        return -1;
    }

    cout << "Server listening on port " << dfNETWORK_PORT << "\n";

    // 메인 루프
    using clock = std::chrono::high_resolution_clock;
    auto lastLogic = clock::now();
    double accumulator = 0.0;
    const double logicInterval = 1.0 / LOGIC_FRAME;

    while (!g_bShutdown) {
        ProfileBegin(f1);
        netIOProcess();
        ProfileEnd(f1);


        static int lastTime = timeGetTime();
        int curTime = timeGetTime();
        int difTime = curTime - lastTime;
        if (difTime >= LOGIC_FRAME_TO_MS)
        {
            int frame_cnt = difTime / LOGIC_FRAME_TO_MS;
            ProfileBegin(f3);
            Update(frame_cnt);
            ProfileEnd(f3);
            lastTime += frame_cnt * LOGIC_FRAME_TO_MS;
        }


        

         ProfileBegin(f2);
         DeleteDieCharacter();
        ProfileEnd(f2);

        ProfileDataOutText(L"Profile.txt");
    }

    closesocket(g_ListenSocket);
    WSACleanup();
    timeEndPeriod(1);
    return 0;
}

// --------------------- 네트워크 I/O ---------------------
void netIOProcess() {

    long long quotientOfDivide64;
    quotientOfDivide64 = MAX_CLIENTS / FD_SETSIZE + 1;

    for (long long i = 0; i < quotientOfDivide64; i++)
    {
        FD_SET readSet, writeSet;
        FD_ZERO(&readSet);
        FD_ZERO(&writeSet);

        FD_SET(g_ListenSocket, &readSet);

        for (long long j = 0; j < FD_SETSIZE; j++)
        {
            long long index = FD_SETSIZE * i + j;
            if (index >= MAX_CLIENTS) continue;
            if (g_SessionArray[index] == nullptr) continue;

            SOCKETINFO* s = g_SessionArray[index];
            if (s && s->sock != INVALID_SOCKET) {
                if (s->sendBuf.GetUseSize() > 0) FD_SET(s->sock, &writeSet);
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
        for (long long j = 0; j < FD_SETSIZE; j++)
        {
            long long index = FD_SETSIZE * i + j;
            if (index >= MAX_CLIENTS) continue;
            SOCKETINFO* pSession = g_SessionArray[index];
            if (pSession == nullptr) continue;

            if (pSession && pSession->sock != INVALID_SOCKET) {
                if (FD_ISSET(pSession->sock, &readSet)) {
                    netProc_Recv(pSession);
                }
            }

            if (pSession && pSession->sock != INVALID_SOCKET) {
                if (FD_ISSET(pSession->sock, &writeSet)) {
                    netProc_Send(pSession);
                }
            }
        }

    }
}

// accept
void netProc_Accept() {
    sockaddr_in clientAddr;
    int addrlen = sizeof(clientAddr);

    while (1)
    {
        SOCKET clientSock = accept(g_ListenSocket, (sockaddr*)&clientAddr, &addrlen);
        if (clientSock == INVALID_SOCKET) {
            int e = WSAGetLastError();
            if (e != WSAEWOULDBLOCK) cerr << "accept failed: " << e << "\n";
            break;
        }

        // TCP_NODELAY (Nagle 비활성화)
        int opt_val = 1;
        setsockopt(clientSock, IPPROTO_TCP, TCP_NODELAY, (const char*)&opt_val, sizeof(opt_val));

        // 송수신 버퍼 크기 증가
        int bufSize = 65536;  // 64KB
        setsockopt(clientSock, SOL_SOCKET, SO_SNDBUF, (char*)&bufSize, sizeof(bufSize));
        setsockopt(clientSock, SOL_SOCKET, SO_RCVBUF, (char*)&bufSize, sizeof(bufSize));

        // 논블로킹
        u_long on = 1;
        ioctlsocket(clientSock, FIONBIO, &on);

        LINGER lingerOption;
        lingerOption.l_onoff = 1;  // Linger 옵션 활성화
        lingerOption.l_linger = 0; // 대기 시간을 0으로 설정 (즉시 RST 송신)

        if (setsockopt(clientSock, SOL_SOCKET, SO_LINGER, (char*)&lingerOption, sizeof(lingerOption)) == SOCKET_ERROR) {
            int err = WSAGetLastError();
            if (err != WSAEWOULDBLOCK) cerr << "setsockopt failed: " << err << "\n";
            break;
        }

        SOCKETINFO* s;
        DWORD id = g_sessionMap->AddSession(s);
        s->sock = clientSock;
        s->session_id = id;
        s->dwLastRecvTime = timeGetTime();
        if (s->pCharacter != nullptr)
        {
            while (1)
            {
                cout << "메모리풀 재할당 오류, 캐릭터 삭제 안됨.\n";
            }
        }

        //세션 배열에 세션 추가하기
        if (!g_DeletedSessionArrayIndex.empty())
        {
            int top = g_DeletedSessionArrayIndex.top();
            g_DeletedSessionArrayIndex.pop();

            // 1. top 인덱스 범위 체크 추가
            if (top >= 0 && top < MAX_CLIENTS)
            {
                g_SessionArray[top] = s;
                s->Arrayindex = top;
            }
            else
            {
                // 서버 수용량 초과 처리 (로그 출력 등)
                while (1)
                {
                    printf("[Error] 스택에 이상한 겂\n");
                }
            }
        }
        else
        {
            // 2. 현재 사이즈가 최대치를 넘지 않는지 반드시 체크
            if (g_CurSessionArraySize < MAX_CLIENTS)
            {
                g_SessionArray[g_CurSessionArraySize] = s;
                s->Arrayindex = g_CurSessionArraySize;
                g_CurSessionArraySize++;
            }
            else
            {
                while (1)
                {
                    // 서버 수용량 초과 처리 (로그 출력 등)
                    printf("[Error] Session Array Full!\n");
                }
            }
        }


        c_CHARACTER* character = ChacracterPool.Alloc();
        character->OnAccept();
        if (character->pSession != nullptr)
        {
            while (1)
            {
                cout << "메모리풀 재할당 오류, 세션 삭제 안됨.\n";
            }
        }
        s->pCharacter = character;
        character->pSession = s;
        character->dwSessionID = id;
        

        //cout << "Accepted new client (session " << s->session_id << ")\n";
        //신규 클라이언트에게 자기 캐릭터 할당 패킷 전송
        CPacket* pPacket = CPacketPool.Alloc();
        pPacket->onAccept();

        clientPacketProc_CREATE_MY_CHARACTER(s, pPacket);

        //섹터 맵 추가 > 다른 클라이언트의 캐릭터 생성 > 다른 클라이언트에게 클라이언트의 캐릭터 생성
        AddCharacterToSectorMap(character, pPacket);
        pPacket->onRelease();
        CPacketPool.Free(pPacket);
    }
}

// recv
void netProc_Recv(SOCKETINFO*& pSession) {
    if (!pSession) return;
    
    int recvCapacity = pSession->recvBuf.DirectEnqueueSize();
    int ret = recv(pSession->sock, pSession->recvBuf.GetRearBufferPtr(), recvCapacity, 0);
    if (ret == 0) {
        Disconnect(pSession);
        return;
    }
    else if (ret == SOCKET_ERROR) {
        int e = WSAGetLastError();
        if (e == WSAEWOULDBLOCK) {
            while (1)
            {
                cout << "[WSAEWOULDBLOCK] 수신불간데 어캐 select했누?\n";
            }
             //수신버퍼에 읽을게 없음
            return;
        }
        else
        {
            Disconnect(pSession);
            return;
        }
    }
    else {
        // 링버퍼에 저장 (Enqueue)
        int enq = pSession->recvBuf.MoveRear(ret);
        if (enq != ret)
        {
            while (1)
            {
                cout << "[netProc_Recv] 링버퍼 수신 실패!\n";
            }
        }
        pSession->dwLastRecvTime = timeGetTime();
    }

    CPacket* pPacket = CPacketPool.Alloc();
    pPacket->onAccept();
   
    // 완성된 패킷이 있으면 처리
    while (pSession->recvBuf.GetUseSize() >= (int)sizeof(st_PACKET_HEADER)) {

        st_PACKET_HEADER hdr;
        int peeked = pSession->recvBuf.Peek((char*)&hdr, (int)sizeof(hdr));
        if (peeked < (int)sizeof(hdr)) break; 
        if (hdr.byCode != dfPACKET_CODE) {
            while (1)
            {
                cerr << "[net_Recv] Bad packet code from session " << pSession->session_id << "\n";
            }
            Disconnect(pSession);
            return;
        }
        // 아직 메시지 문장 전체가 도착 안함
        if (pSession->recvBuf.GetUseSize() < (int)sizeof(hdr) + hdr.bySize) break;

        //문장 전체가 도착했을 경우 일단 헤더만큼 제거
        pSession->recvBuf.MoveFront(sizeof(st_PACKET_HEADER));

        pPacket->Clear();
        // 한번에 꺼낼 수 있는 경우라면 ZeroCopy를 위해 링버퍼 포인터 넘겨주기
        if (pSession->recvBuf.DirectDequeueSize() >= hdr.bySize)
        {
            pPacket->m_chpBuffer =  pSession->recvBuf.GetFrontBufferPtr();
            ret = pPacket->MoveWritePos(hdr.bySize);
            if (ret != hdr.bySize)
            {
                while (1)
                {
                    cerr << "[net_Recv] MoveReadPos size mismatch\n";
                }
            }
        }
        //아니라면 패킷에 담기
        else
        {
            int deq = pSession->recvBuf.Dequeue(pPacket->GetBufferPtr(), hdr.bySize);
            if (deq != hdr.bySize) {
                while (1)
                {
                    cerr << "[net_Recv] Dequeue size mismatch\n";
                }
                Disconnect(pSession);
                return;
            }
            ret = pPacket->MoveWritePos(deq);
            if (ret != deq)
            {
                while (1)
                {
                    cerr << "[net_Recv] MoveReadPos size mismatch\n";
                }
            }
        }
        pSession->recvBuf.MoveFront(hdr.bySize);


        if (!PacketProc(pSession, hdr.byType, pPacket)) {
            while (1)
            {
                cout << "[Send] error\n";
            }
            Disconnect(pSession);
            return;
        }
    }
    pPacket->onRelease();
    CPacketPool.Free(pPacket);
}

// send: SendQ -> 실제 send (Peek -> send -> Dequeue(sent))
void netProc_Send(SOCKETINFO*& pSession) {
    if (!pSession) return;
    int AllDataSize = pSession->sendBuf.GetUseSize();
    if (AllDataSize <= 0) return;


    WSABUF wsabuf[2];
    int bufcnt = pSession->sendBuf.putWsabufData(wsabuf);
    DWORD bytesSent = 0;
    int result = WSASend(pSession->sock, wsabuf, bufcnt, &bytesSent, 0, NULL, NULL);

    if (bytesSent == AllDataSize)
    {
        int dec = pSession->sendBuf.MoveFront(bytesSent);
        if (dec != bytesSent) {
            while (1)
            {
                cout << "[Send] 송신 버퍼에서 끄집어낸 크기가 달라짐\n";
            }
            return;
        }
    }
    else
    {
        if (result == SOCKET_ERROR) {
            int e = WSAGetLastError();
            if (e == WSAEWOULDBLOCK)
            {
                while (1)
                {
                    cout << "[netProc_Send]WSAEWOULDBLOCK: 수신불간데 어캐 select했누?\n";
                }
                Disconnect(pSession);
                return;
            }
            else if (e == WSA_IO_PENDING)
            {
                while (1)
                {
                    cout << "비동기 처리가 일어났다고라??\n";
                }
            }
            else
            {
                Disconnect(pSession);
                return;
            }
        }
        else
        {
            while(1)
            {
                cout << "[netProc_Send]도당최 이게무슨 경우지??\n";
            }
        }
    }
}

//캐릭터와 세션에 플래그 설정
void Disconnect(SOCKETINFO* pSession) {

    if (pSession->Active == false) return;
    pSession->Active = false;

    if (pSession->pCharacter == nullptr)
    {
        while (1)
        {
            cout << "[Disconnect] 벌써 pCharacter 삭제됨";
        }
        return;
    }
    pSession->pCharacter->IsDie = true;
    g_DieCharacterID.push(pSession->pCharacter);
}

void DeleteDieCharacter()
{
    while (!g_DieCharacterID.empty())
    {
        c_CHARACTER* DieCharacter = g_DieCharacterID.top();
        g_DieCharacterID.pop();

        //캐릭터 삭제 패킷 전송
        CPacket* pPacket = CPacketPool.Alloc();
        mpDelete(pPacket, DieCharacter->dwSessionID);
        DeleteCharacterFromSectorMap(DieCharacter, pPacket, &DieCharacter->CurSector);
        pPacket->onRelease();
        CPacketPool.Free(pPacket);

        //섹션 삭제
        SOCKETINFO* session = DieCharacter->pSession;
        //if (session)
        //{
        //    session->pCharacter = nullptr;
        //}
        //DieCharacter->pSession = nullptr;


        // 2. 세션 인덱스 회수
        int index = session->Arrayindex;
        g_DeletedSessionArrayIndex.push(index);
        g_SessionArray[index] = nullptr;

        if (DieCharacter->IsDie != true)
        {
            while (1)
            {
                cout << "[DeleteDieCharacter] 얘 왜 죽임??";
            }
        }


        SOCKADDR_IN clientaddr;
        int addrlen = sizeof(clientaddr);
        getpeername(session->sock, (SOCKADDR*)&clientaddr, &addrlen);
        g_sessionMap->deleteSession(session, inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

        DieCharacter->OnRelease();
        ChacracterPool.Free(DieCharacter);
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

    if (pCharacter->dwSessionID != pCharacter->pSession->session_id)
    {
        cout << "[DeleteCharacter] dwSessionID와 session_id가 다름!\n";
    }
    //섹터 맵에서 캐릭터 검색
    auto it = g_Sector[sectorY][sectorX].find(pCharacter->dwSessionID);
    if (it != g_Sector[sectorY][sectorX].end())
    {
        //섹터 맵 리스트에서 지우기
        g_Sector[sectorY][sectorX].erase(it);
        //printf("[DeleteCharacterAtSectorMap] session_id : %d SectorY :  %d SectorX : %d\n",
           // pCharacter->dwSessionID, pCharacter->CurSector.iY, pCharacter->CurSector.iX);

      
        
        //현재 섹터 기준으로 삭제 패킷 전송(연결 끊김, hp == 0 인 경우)
        if (pSector == &pCharacter->CurSector)
        {
            //먼저 자신의 패킷 보내기
            //SendPacket_Unicast(pCharacter->pSession, pPacket);
            //주위 섹터에 적용
            SendPacket_Around(pCharacter->pSession, pPacket, false, &pCharacter->CurSectorRange);
            LOG_PACKET("DELETE_CHARACTER", pPacket);
        }
        //과거 섹터 기준으로 삭제 패킷 전송(이동에 의한 삭제)
        else if(pSector == &pCharacter->OldSector)
        {
            if (pCharacter->OldSector.index == -1)
            {
                while (1)
                {
                    printf("[DeleteCharacterFromSectorMap] 이동 삭제인데 oldSector = -1\n");
                }
            }
            int changedIndex = pCharacter->CurSector.index - pCharacter->OldSector.index;
            c_SECTOR_AROUND* oldrange = &pCharacter->OldSectorRange;
            SOCKETINFO* pSession = pCharacter->pSession;
            switch (changedIndex)
            {
            case sector_LU:
                //과거 섹터 기준 25678섹터만 전송
                SendPacket_SectorOne(&oldrange->Around[2], pPacket, pSession);
                SendPacket_SectorOne(&oldrange->Around[5], pPacket, pSession);
                SendPacket_SectorOne(&oldrange->Around[6], pPacket, pSession);
                SendPacket_SectorOne(&oldrange->Around[7], pPacket, pSession);
                SendPacket_SectorOne(&oldrange->Around[8], pPacket, pSession);

                SendOtherCharaterDeletePacket_Unicast(pSession, pPacket, &oldrange->Around[2]);
                SendOtherCharaterDeletePacket_Unicast(pSession, pPacket, &oldrange->Around[5]);
                SendOtherCharaterDeletePacket_Unicast(pSession, pPacket, &oldrange->Around[6]);
                SendOtherCharaterDeletePacket_Unicast(pSession, pPacket, &oldrange->Around[7]);
                SendOtherCharaterDeletePacket_Unicast(pSession, pPacket, &oldrange->Around[8]);
                break;
            case sector_UU:
                //678
                SendPacket_SectorOne(&oldrange->Around[6], pPacket, pSession);
                SendPacket_SectorOne(&oldrange->Around[7], pPacket, pSession);
                SendPacket_SectorOne(&oldrange->Around[8], pPacket, pSession);

                SendOtherCharaterDeletePacket_Unicast(pSession, pPacket, &oldrange->Around[6]);
                SendOtherCharaterDeletePacket_Unicast(pSession, pPacket, &oldrange->Around[7]);
                SendOtherCharaterDeletePacket_Unicast(pSession, pPacket, &oldrange->Around[8]);
                break;
            case sector_RU:
                //03678
                SendPacket_SectorOne(&oldrange->Around[0], pPacket, pSession);
                SendPacket_SectorOne(&oldrange->Around[3], pPacket, pSession);
                SendPacket_SectorOne(&oldrange->Around[6], pPacket, pSession);
                SendPacket_SectorOne(&oldrange->Around[7], pPacket, pSession);
                SendPacket_SectorOne(&oldrange->Around[8], pPacket, pSession);

                SendOtherCharaterDeletePacket_Unicast(pSession, pPacket, &oldrange->Around[0]);
                SendOtherCharaterDeletePacket_Unicast(pSession, pPacket, &oldrange->Around[3]);
                SendOtherCharaterDeletePacket_Unicast(pSession, pPacket, &oldrange->Around[6]);
                SendOtherCharaterDeletePacket_Unicast(pSession, pPacket, &oldrange->Around[7]);
                SendOtherCharaterDeletePacket_Unicast(pSession, pPacket, &oldrange->Around[8]);
                break;
            case sector_LL:
                //258
                SendPacket_SectorOne(&oldrange->Around[2], pPacket, pSession);
                SendPacket_SectorOne(&oldrange->Around[5], pPacket, pSession);
                SendPacket_SectorOne(&oldrange->Around[8], pPacket, pSession);

                SendOtherCharaterDeletePacket_Unicast(pSession, pPacket, &oldrange->Around[2]);
                SendOtherCharaterDeletePacket_Unicast(pSession, pPacket, &oldrange->Around[5]);
                SendOtherCharaterDeletePacket_Unicast(pSession, pPacket, &oldrange->Around[8]);
                break;
            case sector_SAME:
                while (1)
                {
                    cout << "[DeleteCharacterFromSectorMap] 어떻게 이런 이동이 가능했지?1";
                }
                break;
            case sector_RR:
                //036
                SendPacket_SectorOne(&oldrange->Around[0], pPacket, pSession);
                SendPacket_SectorOne(&oldrange->Around[3], pPacket, pSession);
                SendPacket_SectorOne(&oldrange->Around[6], pPacket, pSession);

                SendOtherCharaterDeletePacket_Unicast(pSession, pPacket, &oldrange->Around[0]);
                SendOtherCharaterDeletePacket_Unicast(pSession, pPacket, &oldrange->Around[3]);
                SendOtherCharaterDeletePacket_Unicast(pSession, pPacket, &oldrange->Around[6]);
                break;
            case sector_RD:
                //01236
                SendPacket_SectorOne(&oldrange->Around[0], pPacket, pSession);
                SendPacket_SectorOne(&oldrange->Around[1], pPacket, pSession);
                SendPacket_SectorOne(&oldrange->Around[2], pPacket, pSession);
                SendPacket_SectorOne(&oldrange->Around[3], pPacket, pSession);
                SendPacket_SectorOne(&oldrange->Around[6], pPacket, pSession);

                SendOtherCharaterDeletePacket_Unicast(pSession, pPacket, &oldrange->Around[0]);
                SendOtherCharaterDeletePacket_Unicast(pSession, pPacket, &oldrange->Around[1]);
                SendOtherCharaterDeletePacket_Unicast(pSession, pPacket, &oldrange->Around[2]);
                SendOtherCharaterDeletePacket_Unicast(pSession, pPacket, &oldrange->Around[3]);
                SendOtherCharaterDeletePacket_Unicast(pSession, pPacket, &oldrange->Around[6]);
                break;
            case sector_DD:
                //012
                SendPacket_SectorOne(&oldrange->Around[0], pPacket, pSession);
                SendPacket_SectorOne(&oldrange->Around[1], pPacket, pSession);
                SendPacket_SectorOne(&oldrange->Around[2], pPacket, pSession);

                SendOtherCharaterDeletePacket_Unicast(pSession, pPacket, &oldrange->Around[0]);
                SendOtherCharaterDeletePacket_Unicast(pSession, pPacket, &oldrange->Around[1]);
                SendOtherCharaterDeletePacket_Unicast(pSession, pPacket, &oldrange->Around[2]);
                break;
            case sector_LD:
                //01258
                SendPacket_SectorOne(&oldrange->Around[0], pPacket, pSession);
                SendPacket_SectorOne(&oldrange->Around[1], pPacket, pSession);
                SendPacket_SectorOne(&oldrange->Around[2], pPacket, pSession);
                SendPacket_SectorOne(&oldrange->Around[5], pPacket, pSession);
                SendPacket_SectorOne(&oldrange->Around[8], pPacket, pSession);

                SendOtherCharaterDeletePacket_Unicast(pSession, pPacket, &oldrange->Around[0]);
                SendOtherCharaterDeletePacket_Unicast(pSession, pPacket, &oldrange->Around[1]);
                SendOtherCharaterDeletePacket_Unicast(pSession, pPacket, &oldrange->Around[2]);
                SendOtherCharaterDeletePacket_Unicast(pSession, pPacket, &oldrange->Around[5]);
                SendOtherCharaterDeletePacket_Unicast(pSession, pPacket, &oldrange->Around[8]);
                break;
            default:
                while (1)
                {
                    cout << "[DeleteCharacterFromSectorMap] 어떻게 이런 이동이 가능했지?2";
                }
                break;
            }
            LOG_PACKET("DELETE_CHARACTER", pPacket);

            //섹터 변경한 클라에게 과거 섹터 주변 클라이언트들 삭제 패킷 전송

        }
        else
        {
            while (1)
            {
                printf("[DeleteCharacterFromSectorMap] 말도 안되는 섹터 타입을 보냄");
            }
        }
        
    }
    else
    {
        //accept하기 전, 삭제가 일어난 경우 -> 말이 안됨.
        //accept한 후라면 무조건 섹터 배정이 발생했고, 이후엔 여기로 올 수 없음.
        while (1)
        {
            printf("[DeleteCharacterFromSectorMap] 지우려고 하는 캐릭터가 이미 맵에 없음");
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
            //맵 밖을 벗어나는 경우 제외
            if (pSectorRange->Around[i].index == -1) continue;
            for (const auto& pair : g_Sector[pSectorRange->Around[i].iY][pSectorRange->Around[i].iX]) {
                c_CHARACTER* otherCharacter = pair.second;
                if (otherCharacter->IsDie || otherCharacter ->pSession->Active == false) continue;
                int enq = otherCharacter->pSession->sendBuf.Enqueue(pPacket->GetBufferPtr(), pPacket->GetDataSize());
                if (enq != pPacket->GetDataSize())
                {
                    while (1)
                    {
                        cout << "[SendPacket_Around] enq실패!\n";
                    }
                }
            }
        }
    }
    else
    {
        for (int i = 0; i < 9; i++)
        {
            //맵 밖을 벗어나는 경우 제외
            if (pSectorRange->Around[i].index == -1) continue;
            for (const auto& pair : g_Sector[pSectorRange->Around[i].iY][pSectorRange->Around[i].iX]) {
                c_CHARACTER* otherCharacter = pair.second;
                if (otherCharacter->IsDie || otherCharacter->pSession->Active == false|| otherCharacter->pSession == psession) continue;
                int enq = otherCharacter->pSession->sendBuf.Enqueue(pPacket->GetBufferPtr(), pPacket->GetDataSize());
                if (enq != pPacket->GetDataSize())
                {
                    while (1)
                    {
                        cout << "[SendPacket_Around] enq실패!\n";
                    }
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

void Update(int frame)
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

    for (int i = 0; i < MAX_CLIENTS;i++)
    {
        SOCKETINFO* pSession = g_SessionArray[i];
        if (pSession == nullptr|| pSession->Active == false|| pSession->pCharacter->IsDie) continue;

        pCharacter = pSession->pCharacter;
        if (pCharacter->IsDie) continue;

        if (0 >= pCharacter->chHP)
        {   
            //이렇게 해야 송신 버퍼에 들어감.
            CPacket* pPacket = CPacketPool.Alloc();
            
            pCharacter->IsDie = true;
            g_DieCharacterID.push(pCharacter);
            pPacket->onRelease();
            CPacketPool.Free(pPacket);
            // 사망처리. 
            //Disconnect(pCharacter->pSession);
        }
        else
        {
            //int dwCurrentTick = timeGetTime();
            // //일정 시간동안 수신이 없으면 종료처리 
            //if (dwCurrentTick - pCharacter->pSession->dwLastRecvTime >
            //    dfNETWORK_PACKET_RECV_TIMEOUT)
            //{
            //    pCharacter->IsDie = true;
            //    g_DieCharacterID.push(pCharacter->pSession->session_id);
            //    continue;
            //}

            //----------------------------------------------------------- 
            // 현재 동작에 따른 처리. 
            //----------------------------------------------------------- 
            int nextX;
            int nextY;
            switch (pCharacter->dwAction)
            {
            case dfPACKET_MOVE_DIR_LL:
                nextX = pCharacter->shX - dfSPEED_PLAYER_X * frame;
                nextY = pCharacter->shY;
                if (CharacterMoveCheck(nextX,nextY))
                {
                    pCharacter->shX = nextX;
                    pCharacter->shY = nextY;
                }
                break;

            case dfPACKET_MOVE_DIR_LU:
                 nextX = pCharacter->shX - dfSPEED_PLAYER_X * frame;
                 nextY = pCharacter->shY - dfSPEED_PLAYER_Y * frame;
                if (CharacterMoveCheck(nextX, nextY))
                {
                    pCharacter->shX = nextX;
                    pCharacter->shY = nextY;
                }
                break;

            case dfPACKET_MOVE_DIR_UU:
                 nextX = pCharacter->shX;
                 nextY = pCharacter->shY - dfSPEED_PLAYER_Y * frame;
                if (CharacterMoveCheck(nextX, nextY))
                {
                    pCharacter->shX = nextX;
                    pCharacter->shY = nextY;
                }
                break;
            case dfPACKET_MOVE_DIR_RU:
                 nextX = pCharacter->shX + dfSPEED_PLAYER_X * frame;
                 nextY = pCharacter->shY - dfSPEED_PLAYER_Y * frame;
                if (CharacterMoveCheck(nextX, nextY))
                {
                    pCharacter->shX = nextX;
                    pCharacter->shY = nextY;
                }
                break;

            case dfPACKET_MOVE_DIR_RR:
                 nextX = pCharacter->shX + dfSPEED_PLAYER_X * frame;
                 nextY = pCharacter->shY;
                if (CharacterMoveCheck(nextX, nextY))
                {
                    pCharacter->shX = nextX;
                    pCharacter->shY = nextY;
                }
                break;

            case dfPACKET_MOVE_DIR_RD:
                 nextX = pCharacter->shX + dfSPEED_PLAYER_X * frame;
                 nextY = pCharacter->shY + dfSPEED_PLAYER_Y * frame;
                if (CharacterMoveCheck(nextX, nextY))
                {
                    pCharacter->shX = nextX;
                    pCharacter->shY = nextY;
                }
                break;

            case dfPACKET_MOVE_DIR_DD:
                 nextX = pCharacter->shX;
                 nextY = pCharacter->shY + dfSPEED_PLAYER_Y * frame;
                if (CharacterMoveCheck(nextX, nextY))
                {
                    pCharacter->shX = nextX;
                    pCharacter->shY = nextY;
                }
                break;

            case dfPACKET_MOVE_DIR_LD:
                 nextX = pCharacter->shX - dfSPEED_PLAYER_X * frame;
                 nextY = pCharacter->shY + dfSPEED_PLAYER_Y * frame;
                if (CharacterMoveCheck(nextX, nextY))
                {
                    pCharacter->shX = nextX;
                    pCharacter->shY = nextY;
                }
                break;
            }
            if (pCharacter->dwAction >= dfPACKET_MOVE_DIR_LL && pCharacter->dwAction <= dfPACKET_MOVE_DIR_LD)
            {
                //이동인 경우 섹터 업데이트를 함. 
                if (IsSectorUpdate(pCharacter))
                {
                    


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

                    CharacterSectorUpdatePacket(pCharacter);

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
        cout << "[PrintPacket] pPacket > 20\n";
        DebugBreak();
    }
}

bool IsSectorUpdate(c_CHARACTER* pCharacter)
{
    c_SECTOR_POS curSector = pCharacter->CurSector;
    int updateIndex = pCharacter->GetUpdateCurSectorIndex();
    if (curSector.index != updateIndex)
    {
        pCharacter->OldSector = curSector;
        pCharacter->UpdateCurSectorRange();
        //printf("[IsSectorUpdate] sessionId : %d , SectorIndex : %d\n", pCharacter->dwSessionID, pCharacter->CurSector.index);
        return true;
    }
    return false;
}

//지금 여기서 섹터 업데이트 순서를 확인해봐야 할 듯
void CharacterSectorUpdatePacket(c_CHARACTER* pCharacter)
{
    CPacket* pPacket = CPacketPool.Alloc();
    mpDelete(pPacket, pCharacter->dwSessionID);
    //과거 섹터에서 플레이어 삭제
    DeleteCharacterFromSectorMap(pCharacter, pPacket, &pCharacter->OldSector);

    pPacket->Clear();
    //현재 섹터 맵에 추가하기
    AddCharacterToSectorMap(pCharacter, pPacket);
    pPacket->onRelease();
    CPacketPool.Free(pPacket);
    
}

//섹터 맵 추가 > 다른 클라이언트의 캐릭터 생성 > 다른 클라이언트에게 클라이언트의 캐릭터 생성
void AddCharacterToSectorMap(c_CHARACTER* pCharacter, CPacket* pPacket)
{
    
    //섹터 맵 추가
    if (pCharacter->dwSessionID != pCharacter->pSession->session_id)
    {
        cout << "[DeleteCharacter] dwSessionID와 session_id가 다름!\n";
    }
    g_Sector[pCharacter->CurSector.iY][pCharacter->CurSector.iX].emplace(pCharacter->dwSessionID, pCharacter);
 /*   printf("[SetCharacterAtSectorMap] session_id : %d SectorY :  %d SectorX : %d  Index: %d\n",
        pCharacter->dwSessionID, pCharacter->CurSector.iY, pCharacter->CurSector.iX, pCharacter->CurSector.index);*/

    //지금이 첫 생성이라면 현재 모든 섹터에 생성
    if (-1 == pCharacter->OldSector.index)
    {
        //새로운 섹터의 다른 클라이언트 캐릭터 생성
        SOCKETINFO* pSession = pCharacter->pSession;
        clientPacketProc_CREATE_OTHER_CHARACTER(pSession, pPacket);

        //다른 클라이언트에게 섹터의 새로운 클라 캐릭터 생성
        mpCreateCharaterToOther(pPacket, 
            pSession->session_id,
            pSession->pCharacter->byDirection,
            pSession->pCharacter->shX,
            pSession->pCharacter->shY,
            pSession->pCharacter->chHP);
        SendPacket_Around(pSession, pPacket, false, &pCharacter->CurSectorRange);
        LOG_PACKET("mpCreateOtherCharater", pPacket);
    }
    //과거의 섹터가 있었던 경우, 움직임 방향에 따라 생성 섹터 결정
    else
    {
        int changedIndex = pCharacter->CurSector.index - pCharacter->OldSector.index;
        c_SECTOR_AROUND* currange = &pCharacter->CurSectorRange;
        SOCKETINFO* pSession = pCharacter->pSession;

        //다른 클라이언트에게 섹터의 새로운 클라 캐릭터 생성 + 움직임 반영
        mpCreateCharaterToOtherAndGiveMovePacket(pPacket, 
            pCharacter->dwSessionID,
            pCharacter->byDirection,
            pCharacter->byMoveDirection,
            pCharacter->shX,
            pCharacter->shY,
            pCharacter->chHP);
        switch (changedIndex)
        {
        case sector_LU:
            //새로운 섹터 기준 01236섹터만 전송
            SendPacket_SectorOne(&currange->Around[0], pPacket, pCharacter->pSession);
            SendPacket_SectorOne(&currange->Around[1], pPacket, pCharacter->pSession);
            SendPacket_SectorOne(&currange->Around[2], pPacket, pCharacter->pSession);
            SendPacket_SectorOne(&currange->Around[3], pPacket, pCharacter->pSession);
            SendPacket_SectorOne(&currange->Around[6], pPacket, pCharacter->pSession);

            //01236 클라만 새로 생성
            SendCreateCharaterPacket_Unicast(pSession, pPacket, &currange->Around[0]);
            SendCreateCharaterPacket_Unicast(pSession, pPacket, &currange->Around[1]);
            SendCreateCharaterPacket_Unicast(pSession, pPacket, &currange->Around[2]);
            SendCreateCharaterPacket_Unicast(pSession, pPacket, &currange->Around[3]);
            SendCreateCharaterPacket_Unicast(pSession, pPacket, &currange->Around[6]);
            break;
        case sector_UU:
            //012
            SendPacket_SectorOne(&currange->Around[0], pPacket, pCharacter->pSession);
            SendPacket_SectorOne(&currange->Around[1], pPacket, pCharacter->pSession);
            SendPacket_SectorOne(&currange->Around[2], pPacket, pCharacter->pSession);

            SendCreateCharaterPacket_Unicast(pSession, pPacket, &currange->Around[0]);
            SendCreateCharaterPacket_Unicast(pSession, pPacket, &currange->Around[1]);
            SendCreateCharaterPacket_Unicast(pSession, pPacket, &currange->Around[2]);
            break;
        case sector_RU:
            //01258
            SendPacket_SectorOne(&currange->Around[0], pPacket, pCharacter->pSession);
            SendPacket_SectorOne(&currange->Around[1], pPacket, pCharacter->pSession);
            SendPacket_SectorOne(&currange->Around[2], pPacket, pCharacter->pSession);
            SendPacket_SectorOne(&currange->Around[5], pPacket, pCharacter->pSession);
            SendPacket_SectorOne(&currange->Around[8], pPacket, pCharacter->pSession);

            SendCreateCharaterPacket_Unicast(pSession, pPacket, &currange->Around[0]);
            SendCreateCharaterPacket_Unicast(pSession, pPacket, &currange->Around[1]);
            SendCreateCharaterPacket_Unicast(pSession, pPacket, &currange->Around[2]);
            SendCreateCharaterPacket_Unicast(pSession, pPacket, &currange->Around[5]);
            SendCreateCharaterPacket_Unicast(pSession, pPacket, &currange->Around[8]);
            break;
        case sector_LL:
            //036
            SendPacket_SectorOne(&currange->Around[0], pPacket, pCharacter->pSession);
            SendPacket_SectorOne(&currange->Around[3], pPacket, pCharacter->pSession);
            SendPacket_SectorOne(&currange->Around[6], pPacket, pCharacter->pSession);

            SendCreateCharaterPacket_Unicast(pSession, pPacket, &currange->Around[0]);
            SendCreateCharaterPacket_Unicast(pSession, pPacket, &currange->Around[3]);
            SendCreateCharaterPacket_Unicast(pSession, pPacket, &currange->Around[6]);
            break;
        case sector_SAME:
            while (1)
            {
                cout << "[AddCharacterToSectorMap] 어떻게 이런 이동이 가능했지?2";
            }
            break;
        case sector_RR:
            //258
            SendPacket_SectorOne(&currange->Around[2], pPacket, pCharacter->pSession);
            SendPacket_SectorOne(&currange->Around[5], pPacket, pCharacter->pSession);
            SendPacket_SectorOne(&currange->Around[8], pPacket, pCharacter->pSession);

            SendCreateCharaterPacket_Unicast(pSession, pPacket, &currange->Around[2]);
            SendCreateCharaterPacket_Unicast(pSession, pPacket, &currange->Around[5]);
            SendCreateCharaterPacket_Unicast(pSession, pPacket, &currange->Around[8]);
            break;
        case sector_RD:
            //25678
            SendPacket_SectorOne(&currange->Around[2], pPacket, pCharacter->pSession);
            SendPacket_SectorOne(&currange->Around[5], pPacket, pCharacter->pSession);
            SendPacket_SectorOne(&currange->Around[6], pPacket, pCharacter->pSession);
            SendPacket_SectorOne(&currange->Around[7], pPacket, pCharacter->pSession);
            SendPacket_SectorOne(&currange->Around[8], pPacket, pCharacter->pSession);

            SendCreateCharaterPacket_Unicast(pSession, pPacket, &currange->Around[2]);
            SendCreateCharaterPacket_Unicast(pSession, pPacket, &currange->Around[5]);
            SendCreateCharaterPacket_Unicast(pSession, pPacket, &currange->Around[6]);
            SendCreateCharaterPacket_Unicast(pSession, pPacket, &currange->Around[7]);
            SendCreateCharaterPacket_Unicast(pSession, pPacket, &currange->Around[8]);
            break;
        case sector_DD:
            //678
            SendPacket_SectorOne(&currange->Around[6], pPacket, pCharacter->pSession);
            SendPacket_SectorOne(&currange->Around[7], pPacket, pCharacter->pSession);
            SendPacket_SectorOne(&currange->Around[8], pPacket, pCharacter->pSession);

            SendCreateCharaterPacket_Unicast(pSession, pPacket, &currange->Around[6]);
            SendCreateCharaterPacket_Unicast(pSession, pPacket, &currange->Around[7]);
            SendCreateCharaterPacket_Unicast(pSession, pPacket, &currange->Around[8]);
            break;
        case sector_LD:
            //03678
            SendPacket_SectorOne(&currange->Around[0], pPacket, pCharacter->pSession);
            SendPacket_SectorOne(&currange->Around[3], pPacket, pCharacter->pSession);
            SendPacket_SectorOne(&currange->Around[6], pPacket, pCharacter->pSession);
            SendPacket_SectorOne(&currange->Around[7], pPacket, pCharacter->pSession);
            SendPacket_SectorOne(&currange->Around[8], pPacket, pCharacter->pSession);

            SendCreateCharaterPacket_Unicast(pSession, pPacket, &currange->Around[0]);
            SendCreateCharaterPacket_Unicast(pSession, pPacket, &currange->Around[3]);
            SendCreateCharaterPacket_Unicast(pSession, pPacket, &currange->Around[6]);
            SendCreateCharaterPacket_Unicast(pSession, pPacket, &currange->Around[7]);
            SendCreateCharaterPacket_Unicast(pSession, pPacket, &currange->Around[8]);
            break;
        default:
            while (1)
            {
                cout << "[AddCharacterToSectorMap] 어떻게 이런 이동이 가능했지?3";
            }
            break;
        }
        LOG_PACKET("DELETE_CHARACTER", pPacket);
    }
}



void SendPacket_SectorOne(c_SECTOR_POS* pSector, CPacket* pPacket, SOCKETINFO* pExceptSession)
{
    if (pSector->index == -1) return;
    int iSectorX = pSector->iX;
    int iSectorY = pSector->iY;
    for (const auto& pair : g_Sector[iSectorY][iSectorX]) {
        c_CHARACTER* otherCharacter = pair.second;
        if (otherCharacter->IsDie || otherCharacter->pSession->Active == false || otherCharacter->pSession == pExceptSession) continue;
        int enq = otherCharacter->pSession->sendBuf.Enqueue(pPacket->GetBufferPtr(), pPacket->GetDataSize());
        if (enq != pPacket->GetDataSize())
        {
            while (1)
            {
                cout << "[SendPacket_Around] enq실패!\n";
            }
        }
    }
}

void SendOtherCharaterDeletePacket_Unicast(SOCKETINFO* pSession, CPacket* pPacket, c_SECTOR_POS* pSector)
{
    if (pSector->index == -1) return;
    int sectorX = pSector->iX;
    int sectorY = pSector->iY;
    //섹터 리스트에 있는 모든 플레이어의 삭제 패킷 전송
    for (const auto& pair : g_Sector[sectorY][sectorX]) {
        c_CHARACTER* otherCharacter = pair.second;
        if (otherCharacter->pSession == pSession || otherCharacter->IsDie) continue;
        mpDelete(pPacket, otherCharacter->pSession->session_id);
        int enq = pSession->sendBuf.Enqueue(pPacket->GetBufferPtr(), pPacket->GetDataSize());
        if (enq != pPacket->GetDataSize())
        {
            while (1)
            {
                cout << "[SendPacket_Around] enq실패!\n";
            }
        }
    }
}

void SendCreateCharaterPacket_Unicast(SOCKETINFO* pSession, CPacket* pPacket, c_SECTOR_POS* pSector)
{
    if (pSector->index == -1) return;

    st_PACKET_HEADER hdr;

    int sectorX = pSector->iX;
    int sectorY = pSector->iY;

    c_CHARACTER* myCharacter = pSession->pCharacter;
    c_CHARACTER* otherCharacter;
    for (const auto& pair : g_Sector[sectorY][sectorX]) {
        otherCharacter = pair.second;

        if (otherCharacter == nullptr || otherCharacter == myCharacter || otherCharacter->IsDie) continue;

        pPacket->Clear();
        hdr.byCode = dfPACKET_CODE;
        hdr.bySize = sizeof(st_SC_CREATE_OTHER_CHARACTER);
        hdr.byType = dfPACKET_SC_CREATE_OTHER_CHARACTER;
        pPacket->PutData((char*)&hdr, sizeof(st_PACKET_HEADER));

        *pPacket << (long)otherCharacter->dwSessionID;
        *pPacket << otherCharacter->byDirection;
        *pPacket << otherCharacter->shX;
        *pPacket << otherCharacter->shY;
        *pPacket << otherCharacter->chHP;

        //다른 캐릭터가 이동 중에 있다면 이를 반영
        if (otherCharacter->dwAction >= dfPACKET_MOVE_DIR_LL && otherCharacter->dwAction <= dfPACKET_MOVE_DIR_LD)
        {
            hdr.bySize = sizeof(st_SC_MOVE_START);
            hdr.byType = dfPACKET_SC_MOVE_START;
            pPacket->PutData((char*)&hdr, sizeof(st_PACKET_HEADER));
            *pPacket << (long)otherCharacter->dwSessionID;
            *pPacket << otherCharacter->byMoveDirection;
            *pPacket << otherCharacter->shX;
            *pPacket << otherCharacter->shY;
        }

        LOG_PACKET("CreateOtherCharacter", pPacket);
        int enqdata = pSession->sendBuf.Enqueue(pPacket->GetBufferPtr(), pPacket->GetDataSize());
        if (enqdata != pPacket->GetDataSize())
        {
            while (1)
            {
                printf("[SendPacket] 송신 버퍼에 넣기 실패");
            }
        }
    }
}

void SendPacket_Unicast(SOCKETINFO* pSession, CPacket* pPacket)
{
    int enq = pSession->sendBuf.Enqueue(pPacket->GetBufferPtr(), pPacket->GetDataSize());
    if (enq != pPacket->GetDataSize())
    {
        while (1)
        {
            cout << "[SendPacket_Unicast] enq 못했음\n";
        }
    }
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
        break;

    default:
        return false;
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
        mpSync(pPacket, pSession->session_id, pSession->pCharacter->shX, pSession->pCharacter->shY);
        SendPacket_Around(pSession, pPacket, true, &pSession->pCharacter->CurSectorRange);
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
    
    //여기서 섹터 업데이트를 한다...?
    //----------------------------------------------------------------------- 
    if (IsSectorUpdate(pSession->pCharacter))
    {
        //----------------------------------------------------------------------- 
        // 섹터가 변경된 경우는 클라에게 관련 정보를 쏜다.
        //----------------------------------------------------------------------- 
        CharacterSectorUpdatePacket(pSession->pCharacter);
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
        mpSync(pPacket, pSession->session_id, pSession->pCharacter->shX, pSession->pCharacter->shY);
        SendPacket_Around(pSession, pPacket, true, &pSession->pCharacter->CurSectorRange);
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
        CharacterSectorUpdatePacket(pSession->pCharacter);
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

     pSession->pCharacter->dwAction = dfPACKET_CS_ATTACK1;

    mpAttack(pPacket, pSession->session_id, pSession->pCharacter->byDirection,
        pSession->pCharacter->shX, pSession->pCharacter->shY, dfPACKET_SC_ATTACK1);
    SendPacket_Around(pSession, pPacket, true, &pSession->pCharacter->CurSectorRange);
    LOG_PACKET("Attack1", pPacket);

    netPacketProc_Damage(pSession, pPacket, dfATTACK1_RANGE_X, dfATTACK1_RANGE_Y, dfATTACK1_DAMAGE);
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

    pSession->pCharacter->dwAction = dfPACKET_CS_ATTACK2;

    pPacket->Clear();
    mpAttack(pPacket, pSession->session_id, pSession->pCharacter->byDirection,
        pSession->pCharacter->shX, pSession->pCharacter->shY, dfPACKET_SC_ATTACK2);
    SendPacket_Around(pSession, pPacket, true, &pSession->pCharacter->CurSectorRange);
    LOG_PACKET("Attack2", pPacket);

    // 공격 범위 내 충돌 검사 및 데미지 처리
    int ATTACK_RANGE_X = dfATTACK2_RANGE_X;
    int ATTACK_RANGE_Y = dfATTACK2_RANGE_Y;



    netPacketProc_Damage(pSession, pPacket, dfATTACK2_RANGE_X, dfATTACK2_RANGE_Y, dfATTACK2_DAMAGE);

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

    pSession->pCharacter->dwAction = dfPACKET_CS_ATTACK3;

    pPacket->Clear();
    mpAttack(pPacket, pSession->session_id, pSession->pCharacter->byDirection,
        pSession->pCharacter->shX, pSession->pCharacter->shY, dfPACKET_SC_ATTACK3);
    SendPacket_Around(pSession, pPacket, true, &pSession->pCharacter->CurSectorRange);
    LOG_PACKET("Attack3", pPacket);


    netPacketProc_Damage(pSession, pPacket, dfATTACK3_RANGE_X, dfATTACK3_RANGE_Y, dfATTACK3_DAMAGE);
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
    int enq = pSession->sendBuf.Enqueue(pPacket->GetBufferPtr(), pPacket->GetDataSize());
    if (enq != pPacket->GetDataSize())
    {
        while (1)
        {
            cout << "[SendPacket_Unicast] enq 못했음\n";
        }
    }
    return true;
}

//캐릭터 주위 9섹터에 범위내 플레이어를 가격하는 패킷 전송
bool netPacketProc_Damage(SOCKETINFO* pSession, CPacket* pPacket, int Attack_XRange, int Attack_YRange, BYTE Damage)
{
    for (int i = 0; i < 9; i++)
    {
        if (pSession->pCharacter->CurSectorRange.Around[i].index != -1)
        {
            //현재 플레이어 주변 섹터를 기준 공격 범위내 모든 플레이어 검색
            for (const auto& pair : g_Sector[pSession->pCharacter->CurSectorRange.Around[i].iY][pSession->pCharacter->CurSectorRange.Around[i].iX]) {
                int sessionID = pair.first;
                SOCKETINFO* target;
                g_sessionMap->GetSessionptr(sessionID, target);
                if (target == nullptr || target == pSession) continue;
                if (target->pCharacter->IsDie) continue;

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

                //방향과 범위가 같은 캐릭터를 대상으로 주위 섹터에 Damage 패킷 전송
                if (dx <= Attack_XRange && rangeDirection && dy <= Attack_YRange) 
                {
                    //closesocket(target->sock);


                    target->pCharacter->chHP -= Damage;

                    pPacket->Clear();
                    //공격 당하는 other의 섹터 주위 플레이어에게 Damage 패킷 전송.
                    mpDamage(pPacket, pSession->session_id, target->session_id, target->pCharacter->chHP);
                    //mpDamage(pPacket, pSession->session_id, target->session_id, (char)18);
                    SendPacket_Around(target, pPacket, true, &target->pCharacter->CurSectorRange);
                    //LOG_PACKET("Damage", pPacket);
                    //cout << "Damged Client Session ID : " << target->session_id << " Client X: " << target->shX << " Y: " << target->shY << " HP:" << (int)target->chHP << "\n";
                    if (target->pCharacter->chHP <= 0)
                    {
                        target->pCharacter->IsDie = true;
                        g_DieCharacterID.push(target->pCharacter);
                    }
                }
            }
        }
    }
    return false;
}

bool netPacketProc_Sync(SOCKETINFO* pSession, CPacket* pPacket)
{
    mpSync(pPacket, pSession->session_id, pSession->pCharacter->shX, pSession->pCharacter->shY);
    int enq = pSession->sendBuf.Enqueue(pPacket->GetBufferPtr(), pPacket->GetDataSize());
    if (enq != pPacket->GetDataSize())
    {
        while (1)
        {
            cout << "[SendPacket_Unicast] enq 못했음\n";
        }
    }
    return false;
}



bool clientPacketProc_CREATE_MY_CHARACTER(SOCKETINFO* pSession, CPacket* pPacket)
{
    pPacket->Clear();
    st_PACKET_HEADER hdr;
    hdr.byCode = dfPACKET_CODE;
    hdr.bySize = sizeof(st_SC_CREATE_MY_CHARACTER);
    hdr.byType = dfPACKET_SC_CREATE_MY_CHARACTER;
    *pPacket << hdr.byCode;
    *pPacket << hdr.bySize;
    *pPacket << hdr.byType;

    *pPacket << (long)pSession->session_id;
    *pPacket << pSession->pCharacter->byDirection;
    *pPacket << pSession->pCharacter->shX;
    *pPacket << pSession->pCharacter->shY;
    *pPacket << pSession->pCharacter->chHP;
    //*pPacket << (char)50;

    LOG_PACKET("CreateMyCharacter", pPacket);
    //cout << "Create Client Session ID : " << pSession->session_id << " Client X: " << pSession->shX << " Y: " << pSession->shY << "\n";
    int enqdata = pSession->sendBuf.Enqueue(pPacket->GetBufferPtr(), pPacket->GetDataSize());
    if (enqdata != pPacket->GetDataSize())
    {
        while (1)
        {
            printf("[SendPacket] 송신 버퍼에 넣기 실패\n");
        }
    }

    return true;
}

bool clientPacketProc_CREATE_OTHER_CHARACTER(SOCKETINFO* pSession, CPacket* pPacket)
{
    st_PACKET_HEADER hdr;

    c_CHARACTER* MyCharacter = pSession->pCharacter;
    for (int i = 0; i < 9; i++)
    {
        int sectorX = MyCharacter->CurSectorRange.Around[i].iX;
        int sectorY = MyCharacter->CurSectorRange.Around[i].iY;
        c_CHARACTER* otherCharacter;
        if (MyCharacter->CurSectorRange.Around[i].index != -1)
        {
            for (const auto& pair : g_Sector[sectorY][sectorX]) {
                otherCharacter = pair.second;
                if (otherCharacter == MyCharacter || otherCharacter->IsDie) continue;

                pPacket->Clear();
                hdr.byCode = dfPACKET_CODE;
                hdr.bySize = sizeof(st_SC_CREATE_OTHER_CHARACTER);
                hdr.byType = dfPACKET_SC_CREATE_OTHER_CHARACTER;
                pPacket->PutData((char*)&hdr, sizeof(st_PACKET_HEADER));

                *pPacket << (long)otherCharacter->dwSessionID;
                *pPacket << otherCharacter->byDirection;
                *pPacket << otherCharacter->shX;
                *pPacket << otherCharacter->shY;
                *pPacket << otherCharacter->chHP;

                //다른 캐릭터가 이동 중에 있다면 이를 반영
                if (otherCharacter->dwAction >= dfPACKET_MOVE_DIR_LL && otherCharacter->dwAction <= dfPACKET_MOVE_DIR_LD)
                {
                    hdr.bySize = sizeof(st_SC_MOVE_START);
                    hdr.byType = dfPACKET_SC_MOVE_START;
                    pPacket->PutData((char*)&hdr, sizeof(st_PACKET_HEADER));
                    *pPacket << (long)otherCharacter->dwSessionID;
                    *pPacket << otherCharacter->byMoveDirection;
                    *pPacket << otherCharacter->shX;
                    *pPacket << otherCharacter->shY;
                }

                LOG_PACKET("CreateOtherCharacter", pPacket);
                int enqdata = pSession->sendBuf.Enqueue(pPacket->GetBufferPtr(), pPacket->GetDataSize());
                if (enqdata != pPacket->GetDataSize())
                {
                    while (1)
                    {
                        printf("[SendPacket] 송신 버퍼에 넣기 실패\n");
                    }
                }
            }
        }
    }

    return true;
}


void mpCreateCharaterToOtherAndGiveMovePacket(CPacket* pPacket, DWORD dwSessionID, BYTE byDir, BYTE moveDir, short shX, short shY, BYTE hp)
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

    stPacketHeader.bySize = sizeof(st_SC_MOVE_START);
    stPacketHeader.byType = dfPACKET_SC_MOVE_START;
    pPacket->PutData((char*)&stPacketHeader, sizeof(st_PACKET_HEADER));
    *pPacket << (long)dwSessionID;
    *pPacket << moveDir;
    *pPacket << shX;
    *pPacket << shY;
}

void mpCreateCharaterToOther(CPacket* pPacket, DWORD dwSessionID, BYTE byDir, short shX, short shY, BYTE hp)
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

void mpDelete(CPacket* pPacket, DWORD dwSessionID)
{
    st_PACKET_HEADER stPacketHeader;
    stPacketHeader.byCode = dfPACKET_CODE;
    stPacketHeader.bySize = sizeof(st_SC_DELETE_CHARACTER);
    stPacketHeader.byType = dfPACKET_SC_DELETE_CHARACTER;
    pPacket->Clear();
    pPacket->PutData((char*)&stPacketHeader, sizeof(st_PACKET_HEADER));
    *pPacket << (long)dwSessionID;
}
