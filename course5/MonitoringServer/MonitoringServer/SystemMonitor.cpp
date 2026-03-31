#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif
#define _WIN32_WINNT 0x0600 // Vista 이상 (FreeMibTable 사용 가능)

#include "SystemMonitor.h"

#include <winsock2.h> // iphlpapi.h 보다 먼저 포함하는 것이 안전합니다.
#include <ws2tcpip.h>
#include <Windows.h>
#include <iphlpapi.h>
#include <pdh.h>
#include <psapi.h>
#include <pdhmsg.h>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "pdh.lib")
#pragma comment(lib, "psapi.lib")

// ============================================================
// PDH (NonPaged Pool)
// ============================================================

static PDH_HQUERY g_query = NULL;
static PDH_HCOUNTER g_nonPagedCounter = NULL;


// ============================================================
// static 변수
// ============================================================

bool SystemMonitor::_initialized = false;

// CPU
unsigned long long SystemMonitor::_lastIdle = 0;
unsigned long long SystemMonitor::_lastKernel = 0;
unsigned long long SystemMonitor::_lastUser = 0;
int SystemMonitor::_cpuTotal = 0;

// Network
unsigned long long SystemMonitor::_lastRecv = 0;
unsigned long long SystemMonitor::_lastSend = 0;
unsigned long long SystemMonitor::_lastTick = 0;
unsigned long long SystemMonitor::_netRecv = 0;
unsigned long long SystemMonitor::_netSend = 0;

// Memory
int SystemMonitor::_nonPaged = 0;
int SystemMonitor::_availMem = 0;


// ============================================================
// 유틸
// ============================================================

static unsigned long long FileTimeToULL(const FILETIME& ft)
{
    return (((unsigned long long)ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
}


// ============================================================
// Network (64bit, 안전)
// ============================================================

static bool GetNetworkTotal(unsigned long long& outRecv, unsigned long long& outSend)
{
    MIB_IF_TABLE2* table = nullptr;

    if (GetIfTable2(&table) != NO_ERROR)
        return false;

    unsigned long long totalRecv = 0;
    unsigned long long totalSend = 0;

    for (ULONG i = 0; i < table->NumEntries; i++)
    {
        const MIB_IF_ROW2& row = table->Table[i];

        // 상태 UP 인터페이스만 (기본 필터)
        if (row.OperStatus != IfOperStatusUp)
            continue;

        totalRecv += row.InOctets;
        totalSend += row.OutOctets;
    }

    FreeMibTable(table);

    outRecv = totalRecv;
    outSend = totalSend;
    return true;
}


// ============================================================
// Initialize
// ============================================================

bool SystemMonitor::Initialize()
{
    if (PdhOpenQuery(NULL, 0, &g_query) != ERROR_SUCCESS)
        return false;

    if (PdhAddCounter(g_query, L"\\Memory\\Pool Nonpaged Bytes", 0, &g_nonPagedCounter) != ERROR_SUCCESS)
        return false;

    // 초기 샘플 2번 (중요)
    PdhCollectQueryData(g_query);
    Sleep(100);  // 샘플 간격 확보
    PdhCollectQueryData(g_query);

    _lastTick = GetTickCount64();

    return true;
}


// ============================================================
// Update
// ============================================================

void SystemMonitor::Update()
{
    unsigned long long now = GetTickCount64();

    // =========================
    // CPU
    // =========================
    FILETIME idle, kernel, user;
    GetSystemTimes(&idle, &kernel, &user);

    unsigned long long i = FileTimeToULL(idle);
    unsigned long long k = FileTimeToULL(kernel);
    unsigned long long u = FileTimeToULL(user);

    if (_initialized)
    {
        unsigned long long idleDiff = i - _lastIdle;
        unsigned long long totalDiff = (k + u) - (_lastKernel + _lastUser);

        if (totalDiff > 0)
        {
            _cpuTotal = (int)((1.0 - (double)idleDiff / totalDiff) * 100.0);
        }
    }

    _lastIdle = i;
    _lastKernel = k;
    _lastUser = u;


    // =========================
    // Available Memory
    // =========================
    PERFORMANCE_INFORMATION pi;
    if (GetPerformanceInfo(&pi, sizeof(pi)))
    {
        SIZE_T availBytes = pi.PhysicalAvailable * pi.PageSize;
        _availMem = (int)(availBytes / 1024); // KB
    }


    // =========================
    // NonPaged Pool (PDH)
    // =========================
    if (g_query)
    {
        if (PdhCollectQueryData(g_query) == ERROR_SUCCESS)
        {
            PDH_FMT_COUNTERVALUE value;

            if (PdhGetFormattedCounterValue(
                g_nonPagedCounter,
                PDH_FMT_LARGE,
                NULL,
                &value) == ERROR_SUCCESS
                && value.CStatus == ERROR_SUCCESS)   // 상태 체크 추가
            {
                _nonPaged = (int)(value.largeValue / (1024 * 1024)); // KB
            }
        }
    }


    // =========================
    // Network (KB/sec)
    // =========================
    unsigned long long totalRecv = 0;
    unsigned long long totalSend = 0;

    if (GetNetworkTotal(totalRecv, totalSend))
    {
        if (_initialized)
        {
            unsigned long long deltaRecv = totalRecv - _lastRecv;
            unsigned long long deltaSend = totalSend - _lastSend;
            unsigned long long deltaTime = now - _lastTick; // ms

            if (deltaTime > 0)
            {
                unsigned long long recvBytesPerSec = (deltaRecv * 1000ULL) / deltaTime;
                unsigned long long sendBytesPerSec = (deltaSend * 1000ULL) / deltaTime;

                _netRecv = recvBytesPerSec / 1024; // KB/sec
                _netSend = sendBytesPerSec / 1024; // KB/sec
            }
        }

        _lastRecv = totalRecv;
        _lastSend = totalSend;
    }

    _lastTick = now;
    _initialized = true;
}


// ============================================================
// Getter
// ============================================================

int SystemMonitor::GetCpuTotal() { return _cpuTotal; }
int SystemMonitor::GetNonPagedMemory() { return _nonPaged; }
int SystemMonitor::GetAvailableMemory() { return _availMem; }

unsigned long long SystemMonitor::GetNetworkRecvBytes() { return _netRecv; }
unsigned long long SystemMonitor::GetNetworkSendBytes() { return _netSend; }