#pragma once
#include "stdafx.h"

class SOCKETINFO;

//----------------------------------------------- 
// 섹터 하나의 좌표 정보 
//----------------------------------------------- 
class c_SECTOR_POS
{
public:
	int iX;
	int iY;
};

//----------------------------------------------- 
// 특정 위치 주변의 9개 섹터 정보 
//----------------------------------------------- 
class c_SECTOR_AROUND
{
	int iCount;
	c_SECTOR_POS Around[9];
};


//--------------------------------------------------------------- 
// 캐릭터 정보 구조체. 
//--------------------------------------------------------------- 
class c_CHARACTER
{
public:
	c_CHARACTER(SOCKETINFO* psession, DWORD sessionID);
	~c_CHARACTER();

	SOCKETINFO* pSession;
	DWORD  dwSessionID;

	DWORD  dwAction;
	BYTE  byDirection;
	BYTE  byMoveDirection;

	short  shX;
	short  shY;

	c_SECTOR_POS CurSector;  // 섹터 파트에서 설명 
	c_SECTOR_POS OldSector;

	char   chHP;
};