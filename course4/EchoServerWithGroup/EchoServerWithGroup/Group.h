#pragma once

#include <vector>
#include <cstdint>

class SOCKETINFO;
class WorkerThread;
class CLanServer;

struct GroupJob
{
    SOCKETINFO* session;
    void* data;     // 실제 구현에서는 Packet* 등으로 교체
};

class Group
{
public:
    Group();
    virtual ~Group();

    // ===== Owner Thread =====
    void SetOwner(WorkerThread* owner);
    WorkerThread* GetOwner() const;

    // ===== Session 관리 =====
    virtual void AddSession(SOCKETINFO* session);
    virtual void RemoveSession(SOCKETINFO* session);

    const std::vector<SOCKETINFO*>& GetSessions() const;

    // ===== WorkerThread가 호출 =====
    void Execute(const GroupJob& job);
    void Update(uint64_t deltaTime);

protected:
    // ===== 컨텐츠가 구현 =====
    virtual void HandleJob(const GroupJob& job) = 0;
    virtual void OnUpdate(uint64_t deltaTime) = 0;

protected:
    WorkerThread* _ownerThread;
    std::vector<SOCKETINFO*> _sessions;

    CLanServer* curServer_ = nullptr;
};