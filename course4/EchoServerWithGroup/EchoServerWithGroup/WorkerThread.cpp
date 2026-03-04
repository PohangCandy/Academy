#include "WorkerThread.h"
#include "Session.h"
#include "Group.h"

#include <algorithm>
#include <chrono>

WorkerThread::WorkerThread()
    : _running(false)
{
}

WorkerThread::~WorkerThread()
{
    Stop();
}

void WorkerThread::Start()
{
    _running = true;
    _thread = std::thread(&WorkerThread::Run, this);
}

void WorkerThread::Stop()
{
    _running = false;

    if (_thread.joinable())
        _thread.join();
}

void WorkerThread::EnqueueJob(const ThreadJob& job)
{
    std::lock_guard<std::mutex> lock(_queueMutex);
    _jobQueue.push(job);
}

void WorkerThread::AddGroup(Group* group)
{
    _groups.push_back(group);
    group->SetOwner(this);
}

void WorkerThread::RemoveGroup(Group* group)
{
    auto it = std::find(_groups.begin(), _groups.end(), group);
    if (it != _groups.end())
    {
        _groups.erase(it);
        group->SetOwner(nullptr);
    }
}

void WorkerThread::RequestGroupChange(SOCKETINFO* session, Group* targetGroup)
{
    if (session == nullptr || targetGroup == nullptr)
        return;

    Group* currentGroup = session->getGroup();
    if (currentGroup == targetGroup)
        return;

    // 기존 그룹에서 제거
    if (currentGroup)
    {
        currentGroup->RemoveSession(session);
    }

    // 새 그룹에 추가
    targetGroup->AddSession(session);

    // 세션의 그룹 포인터 갱신
    session->setGroup(targetGroup);
}

void WorkerThread::Run()
{
    using clock = std::chrono::steady_clock;
    auto lastTime = clock::now();

    while (_running)
    {
        // ===== 1. Job 처리 =====
        while (true)
        {
            ThreadJob job;

            {
                std::lock_guard<std::mutex> lock(_queueMutex);
                if (_jobQueue.empty())
                    break;

                job = _jobQueue.front();
                _jobQueue.pop();
            }

            if (job.targetGroup)
            {
                // GroupJob 형태로 변환해서 전달
                GroupJob groupJob;
                groupJob.session = static_cast<SOCKETINFO*>(job.session);
                groupJob.data = job.data;

                job.targetGroup->Execute(groupJob);
            }
        }

        // ===== 2. Update 호출 =====
        auto now = clock::now();
        uint64_t deltaTime =
            std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTime).count();

        lastTime = now;

        for (Group* group : _groups)
        {
            group->Update(deltaTime);
        }

        // ===== 3. CPU 과점 방지 =====
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}