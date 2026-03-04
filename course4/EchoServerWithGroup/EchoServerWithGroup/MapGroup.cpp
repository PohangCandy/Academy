#include "MapGroup.h"
#include "Session.h"

#include <algorithm>

MapGroup(int mapId)
{
    _mapId = mapId;
}

MapGroup::~MapGroup()
{
}

void MapGroup::AddSession(SOCKETINFO* session)
{
    if (session == nullptr)
        return;

    {
        std::lock_guard<std::mutex> guard(_lock);
        _sessions.push_back(session);
    }

    //session->SetGroup(this);
}

void MapGroup::RemoveSession(SOCKETINFO* session)
{
    if (session == nullptr)
        return;

    {
        std::lock_guard<std::mutex> guard(_lock);

        auto it = std::find(_sessions.begin(), _sessions.end(), session);
        if (it != _sessions.end())
            _sessions.erase(it);
    }
}

uint32_t MapGroup::GetMapId() const
{
    return _mapId;
}

void MapGroup::Broadcast(const char* data, size_t len, SOCKETINFO* except)
{
    std::lock_guard<std::mutex> guard(_lock);

    for (SOCKETINFO* session : _sessions)
    {
        if (session == except)
            continue;

        //session->Send(data, len);
    }
}

void MapGroup::Update()
{
    // 추후 몬스터 AI, 타이머 이벤트, 상태 갱신 등 처리
}