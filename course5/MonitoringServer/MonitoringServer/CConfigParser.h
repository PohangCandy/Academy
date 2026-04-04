#pragma once
#ifndef __CCONFIG_PARSER__
#define __CCONFIG_PARSER__

#include <Windows.h>
#include <unordered_map>
#include <string>

//------------------------------------------------------------
// CConfigParser - INI 스타일 설정 파일 파서
//
// 사용법:
//   CConfigParser config;
//   if (!config.Open("ServerConfig.ini"))
//       return false;
//
//   int port = config.GetInt("ChatServer", "PORT", 21501);
//   const char* ip = config.GetString("Monitor", "IP", "127.0.0.1");
//
// 파일 형식:
//   [SectionName]
//   Key = Value
//   # 주석 또는 ; 주석
//------------------------------------------------------------

class CConfigParser
{
public:
	CConfigParser();
	~CConfigParser();

	// 설정 파일 열기 (파싱)
	bool Open(const char* szFileName);

	// 값 조회 (섹션 + 키)
	const char* GetString(const char* szSection, const char* szKey, const char* szDefault = "");
	int          GetInt(const char* szSection, const char* szKey, int iDefault = 0);
	bool         GetBool(const char* szSection, const char* szKey, bool bDefault = false);

private:
	// "Section.Key" → Value
	std::unordered_map<std::string, std::string> _mapValues;

	// 문자열 앞뒤 공백 제거
	static std::string Trim(const std::string& str);

	// 복합 키 생성: "SECTION.KEY" (대문자 통일)
	static std::string MakeKey(const char* szSection, const char* szKey);
};

#endif
