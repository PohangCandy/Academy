#include "CConfigParser.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

CConfigParser::CConfigParser()
{
}

CConfigParser::~CConfigParser()
{
}

bool CConfigParser::Open(const char* szFileName)
{
	std::ifstream file(szFileName);
	if (!file.is_open())
		return false;

	_mapValues.clear();

	std::string currentSection;
	std::string line;

	while (std::getline(file, line))
	{
		line = Trim(line);

		// 빈 줄 또는 주석
		if (line.empty() || line[0] == '#' || line[0] == ';')
			continue;

		// 섹션 헤더: [SectionName]
		if (line[0] == '[')
		{
			size_t end = line.find(']');
			if (end != std::string::npos)
			{
				currentSection = line.substr(1, end - 1);
				currentSection = Trim(currentSection);
				// 대문자 통일
				std::transform(currentSection.begin(), currentSection.end(),
					currentSection.begin(), ::toupper);
			}
			continue;
		}

		// Key = Value
		size_t eq = line.find('=');
		if (eq == std::string::npos)
			continue;

		std::string key = Trim(line.substr(0, eq));
		std::string value = Trim(line.substr(eq + 1));

		// 인라인 주석 제거: "value # comment" → "value"
		size_t commentPos = value.find('#');
		if (commentPos != std::string::npos)
			value = Trim(value.substr(0, commentPos));
		commentPos = value.find(';');
		if (commentPos != std::string::npos)
			value = Trim(value.substr(0, commentPos));

		if (key.empty())
			continue;

		// 대문자 통일
		std::transform(key.begin(), key.end(), key.begin(), ::toupper);

		std::string fullKey = currentSection + "." + key;
		_mapValues[fullKey] = value;
	}

	file.close();
	return true;
}

const char* CConfigParser::GetString(const char* szSection, const char* szKey, const char* szDefault)
{
	std::string fullKey = MakeKey(szSection, szKey);
	auto it = _mapValues.find(fullKey);
	if (it == _mapValues.end())
		return szDefault;
	return it->second.c_str();
}

int CConfigParser::GetInt(const char* szSection, const char* szKey, int iDefault)
{
	std::string fullKey = MakeKey(szSection, szKey);
	auto it = _mapValues.find(fullKey);
	if (it == _mapValues.end())
		return iDefault;

	const std::string& val = it->second;

	// 0x 접두사 처리 (16진수)
	if (val.size() > 2 && val[0] == '0' && (val[1] == 'x' || val[1] == 'X'))
		return (int)strtol(val.c_str(), nullptr, 16);

	return atoi(val.c_str());
}

bool CConfigParser::GetBool(const char* szSection, const char* szKey, bool bDefault)
{
	std::string fullKey = MakeKey(szSection, szKey);
	auto it = _mapValues.find(fullKey);
	if (it == _mapValues.end())
		return bDefault;

	std::string val = it->second;
	std::transform(val.begin(), val.end(), val.begin(), ::toupper);

	if (val == "TRUE" || val == "1" || val == "YES" || val == "ON")
		return true;
	if (val == "FALSE" || val == "0" || val == "NO" || val == "OFF")
		return false;

	return bDefault;
}

std::string CConfigParser::Trim(const std::string& str)
{
	size_t start = str.find_first_not_of(" \t\r\n");
	if (start == std::string::npos) return "";
	size_t end = str.find_last_not_of(" \t\r\n");
	return str.substr(start, end - start + 1);
}

std::string CConfigParser::MakeKey(const char* szSection, const char* szKey)
{
	std::string section(szSection);
	std::string key(szKey);
	std::transform(section.begin(), section.end(), section.begin(), ::toupper);
	std::transform(key.begin(), key.end(), key.begin(), ::toupper);
	return section + "." + key;
}
