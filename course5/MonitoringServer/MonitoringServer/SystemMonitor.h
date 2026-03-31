#pragma once

class SystemMonitor
{
public:
    static bool Initialize();
    static void Update();

    static int GetCpuTotal();
    static int GetNonPagedMemory();
    static int GetAvailableMemory();

    static unsigned long long GetNetworkRecvBytes(); // Bytes/sec
    static unsigned long long GetNetworkSendBytes(); // Bytes/sec

private:
    static bool _initialized;

    // CPU
    static unsigned long long _lastIdle;
    static unsigned long long _lastKernel;
    static unsigned long long _lastUser;
    static int _cpuTotal;

    // Network
    static unsigned long long _lastRecv;
    static unsigned long long _lastSend;
    static unsigned long long _lastTick;
    static unsigned long long _netRecv; // Bytes/sec
    static unsigned long long _netSend; // Bytes/sec

    // Memory
    static int _nonPaged;
    static int _availMem;
};