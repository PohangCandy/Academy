#include "errlog.h"
#include "stdafx.h"

void err_quit(const char* msg)
{
	LPVOID lpMsgBuf;

	FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
		(LPSTR)&lpMsgBuf, 0, NULL);

	MessageBoxA(NULL, (LPCSTR)lpMsgBuf, msg, MB_ICONERROR);
	LocalFree(lpMsgBuf);
	exit(1);
}

void err_display(const char* msg)
{
	LPVOID lpMsgBuf;

	FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
		(LPSTR)&lpMsgBuf, 0, NULL);

	printf("[%s]", msg);
	printf(" %s", (char*)lpMsgBuf);
	LocalFree(lpMsgBuf);
}
