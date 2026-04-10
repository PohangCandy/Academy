#pragma once

// WinSock2 는 반드시 Windows.h 보다 먼저 와야 한다. 실수로 다른 헤더가
// Windows.h 를 먼저 끌어와도 legacy winsock.h 가 포함되지 않도록 차단.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
// Windows 7 이상의 Win32 API 를 사용할 수 있도록 타겟 버전 명시.
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601   // Windows 7
#endif
// NOTE: NOMINMAX 는 정의하지 않는다. CPacketForMultiThread.cpp 등
// ChatServer 에서 포팅된 코드가 Windows 의 max 매크로에 의존한다.

#include <WinSock2.h>
#include <Windows.h>
#include <string>
#include <stdio.h>
#include <process.h>
