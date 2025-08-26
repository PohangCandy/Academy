#define _WINSOCK_DEPRECATED_NO_WARNINGS

#pragma comment(lib,"ws2_32")
#include <WinSock2.h>
#include <stdlib.h>
#include <stdio.h>
#include <ws2tcpip.h>
#include "TextParser.h"

//#define SERVERIP "127.0.0.1"
//#define SERVERPORT 9000
#define SERVERPORT 10010
#define BUFSIZE 512

//------------------------------------------
// 프로토콜 - 헤더
//------------------------------------------
#pragma pack(push, 1)
struct st_PACKET_HEADER
{
	DWORD dwPacketCode; // 0x11223344 우리의 패킷확인 고정값

	WCHAR szName[32]; // 본인이름, 유니코드 utf-16 NULL 문자 끝
	WCHAR szFileName[128]; // 파일이름, 유니코드 utf-16 NULL 문자 끝
	int iFileSize;
};
#pragma pack(pop)

//-----------------------------------------
//도메인을 IP 주소로 변경하기
//-----------------------------------------
BOOL DomainToIP(const WCHAR* szDomain, IN_ADDR* pAddr)
{
	ADDRINFOW* pAddrInfo;
	SOCKADDR_IN* pSockAddr;
	if (GetAddrInfo(szDomain, L"0", NULL, &pAddrInfo) != 0)
	{
		return FALSE;
	}
	pSockAddr = (SOCKADDR_IN*)pAddrInfo->ai_addr;
	*pAddr = pSockAddr->sin_addr;
	FreeAddrInfo(pAddrInfo);
	return TRUE;
}

//소켓 함수 오류 출력 후 종료
void err_quit(const char* msg)
{
	LPVOID lpMsgbuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgbuf, 0, NULL);
	MessageBox(NULL, (LPCTSTR)lpMsgbuf, (LPCTSTR)msg, MB_ICONERROR);
	LocalFree(lpMsgbuf);
	exit(1);
}

//소켓 함수 오류 출력
void err_display(const char* msg)
{
	LPVOID lpMsgbuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgbuf, 0, NULL);
	printf("[%s] %s", msg, (char*)lpMsgbuf);
	LocalFree(lpMsgbuf);
}

//사용자 정의 데이터 수신 함수
int recvn(SOCKET s, char* buf, int len, int flags)
{
	int received;
	char* ptr = buf;
	int left = len;

	while (left > 0)
	{
		received = recv(s, ptr, left, flags);
		if (received == SOCKET_ERROR)
			return SOCKET_ERROR;

		else if (received == 0)
			break;

		left -= received;
		ptr += received;
	}

	return(len - left);
}

int main()
{
	int retval;

	//윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 1;

	//socket()
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) err_quit("socket()");

	//fileOpen
	CParser parser;
	parser.LoadFile(L"MinKiChan.jpg");


	//making header
	st_PACKET_HEADER Header;
	Header.dwPacketCode = 0x11223344;

	ZeroMemory(Header.szName, sizeof(Header.szName));
	wcsncpy_s(Header.szName, _countof(Header.szName), L"MinKiChan", _TRUNCATE);
	ZeroMemory(Header.szFileName, sizeof(Header.szFileName));
	wcsncpy_s(Header.szFileName, _countof(Header.szFileName), L"MinKiChan.jpg", _TRUNCATE);
	Header.iFileSize = parser.filesize;

	//connect
	SOCKADDR_IN 	serveraddr;
	IN_ADDR 	Addr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	if (!DomainToIP(L"procademyserver.iptime.org", &Addr)) {
		err_quit("DomainToIP() 실패");
	}


	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr = Addr;		// 네트워크 바이트 오더
	serveraddr.sin_port = htons(SERVERPORT);		// 네트워크 바이트 오더

	//connect
	//SOCKADDR_IN serveraddr;
	//ZeroMemory(&serveraddr, sizeof(serveraddr));
	//serveraddr.sin_family = AF_INET;
	//serveraddr.sin_addr.s_addr = inet_addr(SERVERIP);
	//serveraddr.sin_port = htons(SERVERPORT);


	retval = connect(sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) err_quit("connect()");

	//데이터 통신에 사용할 변수
	int totalSize = sizeof(Header) + parser.filesize;
	char* buf = (char*)(malloc)(totalSize);
	int len;
	memcpy_s(buf, totalSize, &Header, sizeof(Header));
	memcpy_s(buf + sizeof(Header), totalSize - sizeof(Header), parser.filedata, parser.filesize);

	int sentTotal = 0;
	int chunkSize = 1000;

	while (sentTotal < totalSize)
	{
		int bytesLeft = totalSize - sentTotal;
		int sendSize = (bytesLeft > chunkSize) ? chunkSize : bytesLeft;

		retval = send(sock, buf + sentTotal, sendSize, 0);
		if (retval == SOCKET_ERROR)
		{
			err_display("send");
			break;
		}

		printf("[TCP 클라이언트] %d 바이트를 보냈습니다.\n", retval);

		sentTotal += retval;

		if (retval == 0)
			break;  // 연결 끊김 등 예외 처리
	}

	//데이터 받기
	char recvbuf[BUFSIZE];  // 예: 512 바이트

	retval = recvn(sock, recvbuf, retval, 0);
	if (retval == SOCKET_ERROR)
	{
		err_display("recv()");
	}

	//받은 데이터 출력
	recvbuf[retval] = '\0';  // 수신 데이터가 문자열일 경우
	printf("[TCP 클라이언트] %d 바이트를 받았습니다.\n", retval);
	printf("[받은 데이터] %s\n", recvbuf);


	Sleep(10000);

	free(buf);

	//closesocket()
	closesocket(sock);

	//윈속 종료
	WSACleanup();
	return 0;
}