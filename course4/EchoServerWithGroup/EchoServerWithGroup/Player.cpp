#include "Player.h"
#include "Session.h"

Player::Player()
    : _playerId(0),
    _isLoggedIn(false),
    _session(nullptr)
{
}

Player::~Player()
{
}

void Player::BindSession(Session* session)
{
    _session = session;
}

Session* Player::GetSession() const
{
    return _session;
}

void Player::SetPlayerId(int id)
{
    _playerId = id;
}

int Player::GetPlayerId() const
{
    return _playerId;
}

void Player::SetName(const std::string& name)
{
    _name = name;
}

const std::string& Player::GetName() const
{
    return _name;
}

void Player::SetLoginState(bool state)
{
    _isLoggedIn = state;
}

bool Player::IsLoggedIn() const
{
    return _isLoggedIn;
}