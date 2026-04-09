#include "CSystemLog.h"
#include <time.h>
#include <stdio.h>

CSystemLog* CSystemLog::_instance = nullptr;

CSystemLog::CSystemLog()
	: _logLevel(LEVEL_DEBUG), _bConsoleOutput(true), _bSplitByLevel(false), _logCount(0)
{
	StringCchCopyW(_szDirectory, 256, L"Log");
}

CSystemLog::~CSystemLog()
{
}

CSystemLog* CSystemLog::GetInstance()
{
	if (_instance == nullptr)
	{
		_instance = new CSystemLog;
	}
	return _instance;
}

void CSystemLog::SetDirectory(const WCHAR* szDirectory)
{
	StringCchCopyW(_szDirectory, 256, szDirectory);
	CreateDirectoryW(_szDirectory, NULL);
}

void CSystemLog::SetLogLevel(en_LOG_LEVEL level)
{
	_logLevel = level;
}

void CSystemLog::SetConsoleOutput(bool bEnable)
{
	_bConsoleOutput = bEnable;
}

void CSystemLog::SetSplitByLevel(bool bEnable)
{
	_bSplitByLevel = bEnable;
}

void CSystemLog::Log(const WCHAR* szType, en_LOG_LEVEL LogLevel, const WCHAR* szStringFormat, ...)
{
	if (LogLevel < _logLevel)
		return;

	long long count = InterlockedIncrement64(&_logCount);

	const WCHAR* szLevel;
	switch (LogLevel)
	{
	case LEVEL_DEBUG:  szLevel = L"DEBUG"; break;
	case LEVEL_ERROR:  szLevel = L"ERROR"; break;
	case LEVEL_SYSTEM: szLevel = L"SYSTM"; break;
	default:           szLevel = L"?????"; break;
	}

	time_t now;
	time(&now);
	struct tm t;
	localtime_s(&t, &now);

	WCHAR szMessage[1024];
	va_list va;
	va_start(va, szStringFormat);
	StringCchVPrintfW(szMessage, 1024, szStringFormat, va);
	va_end(va);

	WCHAR szLogLine[2048];
	StringCchPrintfW(szLogLine, 2048,
		L"[%-10s] [%04d-%02d-%02d %02d:%02d:%02d / %s / %09lld] %s\n",
		szType,
		t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
		t.tm_hour, t.tm_min, t.tm_sec,
		szLevel,
		count,
		szMessage
	);

	if (_bConsoleOutput)
	{
		wprintf(L"%s", szLogLine);
	}

	// 통합 로그 파일: YYYYMM_TYPE.txt (항상 기록)
	WCHAR szFileName[512];
	StringCchPrintfW(szFileName, 512,
		L"%s\\%04d%02d_%s.txt",
		_szDirectory,
		t.tm_year + 1900, t.tm_mon + 1,
		szType
	);

	FILE* fp = nullptr;
	_wfopen_s(&fp, szFileName, L"a");
	if (fp != nullptr)
	{
		fwprintf(fp, L"%s", szLogLine);
		fclose(fp);
	}

	// 레벨별 분리 파일: YYYYMM_TYPE_ERROR.txt 등
	if (_bSplitByLevel)
	{
		StringCchPrintfW(szFileName, 512,
			L"%s\\%04d%02d_%s_%s.txt",
			_szDirectory,
			t.tm_year + 1900, t.tm_mon + 1,
			szType,
			szLevel
		);

		_wfopen_s(&fp, szFileName, L"a");
		if (fp != nullptr)
		{
			fwprintf(fp, L"%s", szLogLine);
			fclose(fp);
		}
	}
}

void CSystemLog::LogHex(const WCHAR* szType, en_LOG_LEVEL LogLevel, const WCHAR* szLog, BYTE* pByte, int iByteLen)
{
	if (LogLevel < _logLevel)
		return;

	int maxBytes = (iByteLen > 256) ? 256 : iByteLen;
	WCHAR szHex[1024];
	szHex[0] = L'\0';

	WCHAR szTemp[8];
	for (int i = 0; i < maxBytes; i++)
	{
		StringCchPrintfW(szTemp, 8, L"%02X ", pByte[i]);
		StringCchCatW(szHex, 1024, szTemp);
	}

	if (iByteLen > 256)
	{
		StringCchCatW(szHex, 1024, L"...");
	}

	Log(szType, LogLevel, L"%s [%d bytes] %s", szLog, iByteLen, szHex);
}
