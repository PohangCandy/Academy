//TextParser.h
#pragma once
#define _CRT_SECURE_NO_WARNINGS
#undef UNICODE
#undef _UNICODE

#include <stdio.h>
#include <Windows.h>
using namespace std;

class CParser
{
public:
	char* filedata = nullptr;
	int* filesize = nullptr;
	char* current = nullptr;

	 CParser() {}

	  bool LoadFile(const char* filename);

	  bool GetNextWord(char** buf, int* length);

	  bool RemoveSpace();

	  bool GetValue(const char* szName, int* ipValue);

	  bool GetCharacter(const char* szName, char* cdata);

	  bool GetStringWord(const char** chppBuffer, int* ipLength);

	  bool GetOneByte(char* buf);

	  bool IsValid();
};
