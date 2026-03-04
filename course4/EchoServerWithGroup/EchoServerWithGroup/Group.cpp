#include "Group.h"
#include "Session.h"
#include "WorkerThread.h"
#include "CLanServer.h"

#include <algorithm>

Group::Group()
    : _ownerThread(nullptr)
{
}

Group::~Group()
{
}

void Group::SetOwner(WorkerThread* owner)
{
    _ownerThread = owner;
}

WorkerThread* Group::GetOwner() const
{
    return _ownerThread;
}

void Group::AddSession(SOCKETINFO* session)
{
    _sessions.push_back(session);
}

void Group::RemoveSession(SOCKETINFO* session)
{
    auto it = std::find(_sessions.begin(), _sessions.end(), session);
    if (it != _sessions.end())
        _sessions.erase(it);
}

const std::vector<SOCKETINFO*>& Group::GetSessions() const
{
    return _sessions;
}

void Group::Execute(const GroupJob& job)
{
    // WorkerThread가 pop한 Job을 여기로 전달
    HandleJob(job);
}

void Group::Update(uint64_t deltaTime)
{
    OnUpdate(deltaTime);
}