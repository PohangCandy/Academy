#pragma once
#include "stdafx.h"

#define PERF_BEGIN(p)  (p.start = timeGetTime())
#define PERF_END(p)    (p.total = (timeGetTime() - p.start))

struct PerfCounter
{
    DWORD start;
    DWORD total;   // ´©Àû ms
};

PerfCounter g_netPerf = {};
PerfCounter g_logicPerf = {};
PerfCounter g_deletePerf = {};