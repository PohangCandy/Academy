#include "AuthGroup.h"
#include "Session.h"
#include "Player.h"
#include "MapGroup.h"
#include "WorkerThread.h"
#include "CLanServer.h"

#include <iostream>

AuthGroup::AuthGroup()
{
}

AuthGroup::~AuthGroup()
{
}

void AuthGroup::HandleJob(const GroupJob& job)
{
    if (!job.session || !job.data)
        return;

    // 첫 2바이트를 패킷 타입이라고 가정
    AuthPacketType type = *(reinterpret_cast<AuthPacketType*>(job.data));

    switch (type)
    {
    case AuthPacketType::LOGIN_REQUEST:
    {
        LoginRequest* request =
            reinterpret_cast<LoginRequest*>(job.data);

        HandleLogin(job.session, request);
        break;
    }

    default:
        // 인증 단계에서 허용되지 않은 패킷
        std::cout << "Invalid packet in AuthGroup\n";
        break;
    }
}

void AuthGroup::OnUpdate(uint64_t deltaTime)
{
    // AuthGroup은 특별한 주기 로직이 없을 수도 있음
    // 필요하다면 로그인 타임아웃 체크 등 가능
    const uint64_t LOGIN_TIMEOUT = 10000; // 10초

    std::vector<SOCKETINFO*> toDisconnect;

    for (SOCKETINFO* session : _sessions)
    {
        if (session->IsLoggedIn())
            continue;

        //uint64_t elapsed = currentTime - session->GetConnectedTime();

        //if (elapsed > LOGIN_TIMEOUT)
        //{
        //    toDisconnect.push_back(session);
        //}
    }

    //for (SOCKETINFO* session : toDisconnect)
    //{
    //    session->Disconnect();
    //}
}

void AuthGroup::HandleLogin(SOCKETINFO* session, LoginRequest* request)
{
    if (!ValidateToken(request->token))
    {
        std::cout << "Token validation failed\n";
        // session->Disconnect();  // 실제 구현 시
        return;
    }

    Player* player = LoadCharacterFromDB(request->characterId);
    if (!player)
    {
        std::cout << "Character load failed\n";
        // session->Disconnect();
        return;
    }

    //session->BindPlayer(player);
    
    //로그인 성공
    //curServer_->SendPacket();
    MoveToMapGroup(session, player);
}

bool AuthGroup::ValidateToken(uint64_t token)
{
    // TODO: 로그인 서버 또는 Redis 확인
    // 지금은 더미 처리
    return (token != 0);
}

Player* AuthGroup::LoadCharacterFromDB(uint32_t characterId)
{
    // TODO: 실제 DB 접근 필요
    // 지금은 더미 객체 생성
    //Player* player = new Player(characterId);
    Player* player = nullptr;
    return player;
}

void AuthGroup::MoveToMapGroup(SOCKETINFO* session, Player* player)
{
    // TODO:
    // 1. DB에서 마지막 mapId 읽어오기
    // 2. 해당 MapGroup 찾기
    // 3. ChangeGroup 로직 수행

    MapGroup* targetGroup = nullptr; // FindMapGroup(player->GetMapId());

    if (!targetGroup)
    {
        std::cout << "Target MapGroup not found\n";
        return;
    }

    // WorkerThread에 그룹 이동 요청 Job을 넣는 구조가 이상적
    // 예:
    GetOwner()->RequestGroupChange(session, targetGroup);

    std::cout << "Login success, moving to MapGroup\n";
}