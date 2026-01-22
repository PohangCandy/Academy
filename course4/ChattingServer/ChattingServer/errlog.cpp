#include "errlog.h"
#include "stdafx.h"

//소켓 생성, 종료하는 응용 프로그램
void err_quit(const char* msg)
{
	//오류 메시지 담을 시작주소
	LPVOID lpMsgBuf;

	FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
		(LPSTR)&lpMsgBuf, 0, NULL);

	//메시지 상자에 출력
	MessageBoxA(NULL, (LPCSTR)lpMsgBuf, msg, MB_ICONERROR);
	LocalFree(lpMsgBuf);
	exit(1);
}

void err_display(const char* msg)
{
	//오류 메시지 담을 시작주소
	LPVOID lpMsgBuf;

	FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
		(LPSTR)&lpMsgBuf, 0, NULL);

	printf("[%s]", msg);
	printf(" %s",(char*)lpMsgBuf);
	LocalFree(lpMsgBuf);
}