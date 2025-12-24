#pragma once
#include "stdafx.h"

class SOCKETINFO;

//----------------------------------------------- 
// 섹터 하나의 좌표 정보 
//----------------------------------------------- 
class c_SECTOR_POS
{
public:
	int iX = -1;
	int iY = -1;
	//한 숫자로 섹터를 특정하기위한 변수
	int index = -1;
};

//----------------------------------------------- 
// 특정 위치 주변의 9개 섹터 정보 
//----------------------------------------------- 
class c_SECTOR_AROUND
{
public:
	//int iCount;
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

	int GetUpdateCurSectorIndex();
	//int UpdateCurSector();
	int UpdateCurSectorRange();

	SOCKETINFO* pSession;
	DWORD  dwSessionID;

	DWORD  dwAction;
	BYTE  byDirection;
	BYTE  byMoveDirection;

	short  shX;
	short  shY;

	c_SECTOR_POS CurSector;  // 섹터 파트에서 설명 
	c_SECTOR_POS OldSector;

	c_SECTOR_AROUND CurSectorRange;
	c_SECTOR_AROUND OldSectorRange;

	char   chHP;
};