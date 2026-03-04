#pragma once

#include <vector>
#include <queue>
#include <mutex>
#include <atomic>
#include <thread>
#include <cstdint>

class Group;

// ===== Thread가 처리할 Job =====
struct ThreadJob
{
    Group* targetGroup;
    void* data;        // 실제 구현에서는 Packet* 등으로 교체
    void* session;     // Session* 등으로 교체 가능
};

class WorkerThread
{
public:
    WorkerThread();
    ~WorkerThread();

    // ===== Thread 제어 =====
    void Start();
    void Stop();

    // ===== Job Push =====
    void EnqueueJob(const ThreadJob& job);

    // ===== Group 관리 =====
    void AddGroup(Group* group);
    void RemoveGroup(Group* group);

private:
    void Run();   // 내부 스레드 루프

private:
    std::thread _thread;
    std::atomic<bool> _running;

    // ===== Job Queue =====
    std::queue<ThreadJob> _jobQueue;
    std::mutex _queueMutex;

    // ===== 담당 Group 목록 =====
    std::vector<Group*> _groups;
};