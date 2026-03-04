#pragma once

#include <vector>
#include <mutex>
#include <cstdint>

#include "Group.h"

class SOCKETINFO;

class MapGroup : public Group
{
public:
    MapGroup(int mapId);
    virtual ~MapGroup();

    // Group 인터페이스 구현
    virtual void AddSession(SOCKETINFO* session) override;
    virtual void RemoveSession(SOCKETINFO* session) override;

    // 맵 정보
    uint32_t GetMapId() const;

    // 브로드캐스트
    void Broadcast(const char* data, size_t len, SOCKETINFO* except = nullptr);

    // 주기적 업데이트용 (선택)
    void Update();

private:
    int _mapId;

    std::vector<SOCKETINFO*> _sessions;
    std::mutex _lock;
};