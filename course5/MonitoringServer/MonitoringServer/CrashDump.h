#pragma once
#ifndef __CRASH_DUMP__
#define __CRASH_DUMP__

#include <Windows.h>
#include <DbgHelp.h>
#include <Psapi.h>
#include <stdio.h>
#include <time.h>

#pragma comment(lib, "DbgHelp.lib")

class CCrashDump
{
public:
	static void Init()
	{
		_DumpCount = 0;

		_invalid_parameter_handler oldHandler;
		_invalid_parameter_handler newHandler;

		newHandler = myInvalidParameterHandler;

		oldHandler = _set_invalid_parameter_handler(newHandler);

		_CrtSetReportMode(_CRT_WARN, 0);
		_CrtSetReportMode(_CRT_ASSERT, 0);
		_CrtSetReportMode(_CRT_ERROR, 0);

		_CrtSetReportHook(_custom_Report_hook);

		_set_purecall_handler(myPurecallHandler);

		SetUnhandledExceptionFilter(MyExceptionFilter);
	}

	static LONG WINAPI MyExceptionFilter(__in PEXCEPTION_POINTERS pExceptionPointer)
	{
		int iWorkingMemory = 0;
		SYSTEMTIME stNowTime;

		long DumpCount = InterlockedIncrement(&_DumpCount);

		// Working Set (Physical Memory) Size
		HANDLE hProcess = 0;
		PROCESS_MEMORY_COUNTERS pmc;

		hProcess = GetCurrentProcess();

		if (NULL == hProcess)
			return 0;

		if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc)))
		{
			iWorkingMemory = (int)(pmc.WorkingSetSize / 1024 / 1024);
		}
		CloseHandle(hProcess);

		// Timestamp
		GetLocalTime(&stNowTime);

		// Create dump directory
		CreateDirectoryW(L"CrashDump", NULL);

		WCHAR filename[MAX_PATH];
		wsprintf(filename,
			L"CrashDump\\Dump_%04d%02d%02d_%02d%02d%02d_%d_%dMB.dmp",
			stNowTime.wYear, stNowTime.wMonth, stNowTime.wDay,
			stNowTime.wHour, stNowTime.wMinute, stNowTime.wSecond,
			DumpCount, iWorkingMemory);

		printf("\n\n");
		wprintf(L"[CrashDump] ===== Crash !!! ===== %s\n", filename);

		HANDLE hDumpFile = ::CreateFileW(
			filename,
			GENERIC_WRITE,
			FILE_SHARE_WRITE,
			NULL,
			CREATE_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			NULL);

		if (hDumpFile != INVALID_HANDLE_VALUE)
		{
			_MINIDUMP_EXCEPTION_INFORMATION MinidumpExceptionInformation;

			MinidumpExceptionInformation.ThreadId = ::GetCurrentThreadId();
			MinidumpExceptionInformation.ExceptionPointers = pExceptionPointer;
			MinidumpExceptionInformation.ClientPointers = TRUE;

			MiniDumpWriteDump(
				GetCurrentProcess(),
				GetCurrentProcessId(),
				hDumpFile,
				MiniDumpWithFullMemory,
				&MinidumpExceptionInformation,
				NULL,
				NULL);

			CloseHandle(hDumpFile);

			wprintf(L"[CrashDump] Dump saved: %s\n", filename);
		}

		return EXCEPTION_EXECUTE_HANDLER;
	}

	static void myInvalidParameterHandler(
		const wchar_t* expression,
		const wchar_t* function,
		const wchar_t* file,
		unsigned int line,
		uintptr_t pReserved)
	{
		Crash();
	}

	static int _custom_Report_hook(int ireposttype, char* message, int* returnvalue)
	{
		Crash();
		return true;
	}

	static void myPurecallHandler(void)
	{
		Crash();
	}

	static void Crash(void)
	{
		int* p = nullptr;
		*p = 0;
	}

	static long _DumpCount;
};

// Static member definition in header (inline for C++17/20)
__declspec(selectany) long CCrashDump::_DumpCount = 0;

#endif
