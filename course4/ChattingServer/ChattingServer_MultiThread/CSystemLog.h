#pragma once
#ifndef __CSYSTEM_LOG__
#define __CSYSTEM_LOG__

#include <Windows.h>
#include <strsafe.h>

class CSystemLog
{
public:
	enum en_LOG_LEVEL
	{
		LEVEL_DEBUG = 0,
		LEVEL_ERROR,
		LEVEL_SYSTEM
	};

	static CSystemLog* GetInstance();

	void SetDirectory(const WCHAR* szDirectory);
	void SetLogLevel(en_LOG_LEVEL level);
	void SetConsoleOutput(bool bEnable);
	void SetSplitByLevel(bool bEnable);
	void Log(const WCHAR* szType, en_LOG_LEVEL LogLevel, const WCHAR* szStringFormat, ...);
	void LogHex(const WCHAR* szType, en_LOG_LEVEL LogLevel, const WCHAR* szLog, BYTE* pByte, int iByteLen);

private:
	CSystemLog();
	~CSystemLog();

	CSystemLog(const CSystemLog&) = delete;
	CSystemLog& operator=(const CSystemLog&) = delete;

	static CSystemLog* _instance;

	WCHAR _szDirectory[256];
	en_LOG_LEVEL _logLevel;
	bool _bConsoleOutput;
	bool _bSplitByLevel;		// true: 레벨별 파일 분리

	alignas(64) long long _logCount;
};

#define SYSLOG_DIRECTORY(dir)    CSystemLog::GetInstance()->SetDirectory(dir)
#define SYSLOG_LEVEL(level)      CSystemLog::GetInstance()->SetLogLevel(level)
#define LOG(type, level, fmt, ...) CSystemLog::GetInstance()->Log(type, level, fmt, __VA_ARGS__)
#define LOG_HEX(type, level, log, ptr, len) CSystemLog::GetInstance()->LogHex(type, level, log, ptr, len)

#endif
