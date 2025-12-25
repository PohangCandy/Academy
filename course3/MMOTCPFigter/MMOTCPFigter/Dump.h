#pragma once
#include <windows.h>
#include <DbgHelp.h>

#pragma comment(lib, "DbgHelp.lib")

LONG WINAPI MyUnhandledExceptionFilter(struct _EXCEPTION_POINTERS* ExceptionInfo) {
    HANDLE hFile = CreateFile(L"ServerCrash.dmp", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE) {
        MINIDUMP_EXCEPTION_INFORMATION dumpInfo;
        dumpInfo.ThreadId = GetCurrentThreadId();
        dumpInfo.ExceptionPointers = ExceptionInfo;
        dumpInfo.ClientPointers = TRUE;

        MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpNormal, &dumpInfo, NULL, NULL);
        CloseHandle(hFile);
    }
    return EXCEPTION_EXECUTE_HANDLER;
}