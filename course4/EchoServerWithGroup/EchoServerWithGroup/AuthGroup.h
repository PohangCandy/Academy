#pragma once

#include "Group.h"
#include <cstdint>

class SOCKETINFO;
class Player;
class MapGroup;

// ===== 패킷 타입 예시 =====
enum class AuthPacketType : uint16_t
{
    LOGIN_REQUEST = 1,
    SELECT_CHARACTER = 2
};

// ===== 로그인 요청 구조 예시 =====
struct LoginRequest
{
    uint64_t token;
    uint32_t characterId;
};

class AuthGroup : public Group
{
public:
    AuthGroup();
    virtual ~AuthGroup();

protected:
    // Group 오버라이드
    virtual void HandleJob(const GroupJob& job) override;
    virtual void OnUpdate(uint64_t deltaTime) override;

private:
    void HandleLogin(SOCKETINFO* session, LoginRequest* request);

    bool ValidateToken(uint64_t token);
    Player* LoadCharacterFromDB(uint32_t characterId);

    void MoveToMapGroup(SOCKETINFO* session, Player* player);
};