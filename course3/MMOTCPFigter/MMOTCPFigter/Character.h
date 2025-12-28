#pragma once
#include "stdafx.h"

class SOCKETINFO;

#define PlayerFirstHP 10
#define PlayerFirstX ((rand() % (dfRANGE_MOVE_RIGHT - dfRANGE_MOVE_LEFT + 1)) + dfRANGE_MOVE_LEFT)
//#define PlayerFirstX 100
#define PlayerFirstY ((rand() % (dfRANGE_MOVE_BOTTOM - dfRANGE_MOVE_TOP + 1)) + dfRANGE_MOVE_TOP)
//#define PlayerFirstY 100

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
	c_CHARACTER();
	~c_CHARACTER();

	void OnAccept();
	void OnRelease();

	int GetUpdateCurSectorIndex();
	//int UpdateCurSector();
	int UpdateCurSectorRange();

	bool IsDie = false;

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