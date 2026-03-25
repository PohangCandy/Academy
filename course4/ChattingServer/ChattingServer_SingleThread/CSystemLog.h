#pragma once
//---------------------------------------------------------------------------------------------
// 텍스트 로그 (시스템 로그)
//
// 용도:
// 개발/운영 과정에서 원하는 문자열을 파일 및 콘솔로 출력.
//
// 사용법:
//   CSystemLog::GetInstance()->SetDirectory(L"Log_ChattingServer");
//   CSystemLog::GetInstance()->SetLogLevel(CSystemLog::LEVEL_DEBUG);
//   LOG(L"Network", CSystemLog::LEVEL_ERROR, L"[IP:%s] [ID:%llu] 비정상 패킷", ip, id);
//
// 파일 저장 규칙:
//   - 파일명: YYYYMM_Type.txt (월별, 타입별 분리)
//   - 매 호출마다 File Open > Write > Close
//   - 로그 카운터로 멀티스레드 환경에서 순서 보장
//---------------------------------------------------------------------------------------------

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

	//----------------------------------------------
	// 로그 저장 폴더 지정
	// 서버 초기화 시 한 번 호출
	//----------------------------------------------
	void SetDirectory(const WCHAR* szDirectory);

	//----------------------------------------------
	// 로그 레벨 지정
	// DEBUG: DEBUG, ERROR, SYSTEM 모두 저장
	// ERROR: ERROR, SYSTEM 저장
	// SYSTEM: SYSTEM만 저장
	//----------------------------------------------
	void SetLogLevel(en_LOG_LEVEL level);

	//----------------------------------------------
	// 콘솔 출력 여부 설정
	//----------------------------------------------
	void SetConsoleOutput(bool bEnable);

	//----------------------------------------------
	// 일반 텍스트 로그
	// szType: 로그 분류 (L"Network", L"Battle" 등)
	// LogLevel: 로그 레벨
	// szStringFormat: 가변인자 포맷 문자열
	//----------------------------------------------
	void Log(const WCHAR* szType, en_LOG_LEVEL LogLevel, const WCHAR* szStringFormat, ...);

	//----------------------------------------------
	// 바이너리 데이터를 Hex 16진수로 남기는 함수
	//----------------------------------------------
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

	// 로그 카운터 (스레드 안전, 전체 로그에 대한 순서 보장)
	alignas(64) long long _logCount;
};

//----------------------------------------------
// 매크로 - 편의용
//----------------------------------------------
#define SYSLOG_DIRECTORY(dir)    CSystemLog::GetInstance()->SetDirectory(dir)
#define SYSLOG_LEVEL(level)      CSystemLog::GetInstance()->SetLogLevel(level)
#define LOG(type, level, fmt, ...) CSystemLog::GetInstance()->Log(type, level, fmt, __VA_ARGS__)
#define LOG_HEX(type, level, log, ptr, len) CSystemLog::GetInstance()->LogHex(type, level, log, ptr, len)

#endif
