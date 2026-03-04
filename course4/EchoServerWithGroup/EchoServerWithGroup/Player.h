#pragma once

#include <string>

class Session;  // 전방 선언 (순환 참조 방지)

class Player
{
public:
    Player();
    ~Player();

    // 세션 연결
    void BindSession(Session* session);
    Session* GetSession() const;

    // 계정 정보
    void SetPlayerId(int id);
    int GetPlayerId() const;

    void SetName(const std::string& name);
    const std::string& GetName() const;

    // 상태 관리
    void SetLoginState(bool state);
    bool IsLoggedIn() const;

private:
    int _playerId;
    std::string _name;

    bool _isLoggedIn;

    Session* _session;   // 이 플레이어가 연결된 세션
};