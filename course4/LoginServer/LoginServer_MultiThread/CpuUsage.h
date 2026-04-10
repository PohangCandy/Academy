#pragma once
#include <Windows.h>

class CpuUsage
{
public:
    CpuUsage();

    // 0 ~ 100 (%)
    int Update();

private:
    ULONGLONG _prevProcTime;
    ULONGLONG _prevSysTime;
    bool _initialized;

private:
    ULONGLONG FileTimeToULL(const FILETIME& ft);
};