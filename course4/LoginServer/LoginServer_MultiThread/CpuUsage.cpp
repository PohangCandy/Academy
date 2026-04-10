#include "CpuUsage.h"

CpuUsage::CpuUsage()
    : _prevProcTime(0)
    , _prevSysTime(0)
    , _initialized(false)
{
}

ULONGLONG CpuUsage::FileTimeToULL(const FILETIME& ft)
{
    return (static_cast<ULONGLONG>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
}

int CpuUsage::Update()
{
    FILETIME ftIdle, ftKernel, ftUser;
    FILETIME ftCreate, ftExit, ftKernelProc, ftUserProc;

    if (!GetSystemTimes(&ftIdle, &ftKernel, &ftUser))
        return 0;

    if (!GetProcessTimes(GetCurrentProcess(), &ftCreate, &ftExit, &ftKernelProc, &ftUserProc))
        return 0;

    ULONGLONG sysTotal = FileTimeToULL(ftKernel) + FileTimeToULL(ftUser);
    ULONGLONG procTotal = FileTimeToULL(ftKernelProc) + FileTimeToULL(ftUserProc);

    if (!_initialized)
    {
        _prevSysTime = sysTotal;
        _prevProcTime = procTotal;
        _initialized = true;
        return 0;
    }

    ULONGLONG sysDelta = sysTotal - _prevSysTime;
    ULONGLONG procDelta = procTotal - _prevProcTime;

    _prevSysTime = sysTotal;
    _prevProcTime = procTotal;

    if (sysDelta == 0)
        return 0;

    double cpu = (double)procDelta / (double)sysDelta;

    SYSTEM_INFO si;
    GetSystemInfo(&si);

    int result = (int)(cpu * 100.0);

    // 안전 보정
    if (result < 0) result = 0;
    if (result > 100) result = 100;

    return result;
}