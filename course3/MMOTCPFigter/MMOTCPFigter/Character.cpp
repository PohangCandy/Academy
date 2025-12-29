#include "Character.h"
#include "Session.h"
#include "Protocol.h"

c_CHARACTER::c_CHARACTER()
{
	pSession = nullptr;
	dwSessionID = -1;

	dwAction = dfPACKET_CS_MOVE_STOP;
	byDirection = dfPACKET_MOVE_DIR_RR;
	byMoveDirection = dfPACKET_CS_MOVE_STOP;
	shX = PlayerFirstX;
	shY = PlayerFirstY;
	chHP = PlayerFirstHP;

	CurSector.iX = shX / dfSECTOR_X_Length;
	CurSector.iY = shY / dfSECTOR_Y_Length;
	CurSector.index = CurSector.iY * dfSECTOR_MAPMAX_X + CurSector.iX;

	UpdateCurSectorRange();

	OldSector.iX = -1;
	OldSector.iY = -1;
	OldSector.index = -1;

	OldSectorRange = { 0 };
}

c_CHARACTER::~c_CHARACTER()
{
	pSession = nullptr;
	dwSessionID = -1;
}

void c_CHARACTER::OnAccept()
{
	IsDie = false;

	pSession = nullptr;
	dwSessionID = -1;

	dwAction = dfPACKET_CS_MOVE_STOP;
	byDirection = dfPACKET_MOVE_DIR_RR;
	byMoveDirection = dfPACKET_CS_MOVE_STOP;
	shX = PlayerFirstX;
	shY = PlayerFirstY;
	chHP = PlayerFirstHP;

	CurSector.iX = shX / dfSECTOR_X_Length;
	CurSector.iY = shY / dfSECTOR_Y_Length;
	CurSector.index = CurSector.iY * dfSECTOR_MAPMAX_X + CurSector.iX;

	UpdateCurSectorRange();

	OldSector.iX = -1;
	OldSector.iY = -1;
	OldSector.index = -1;

	OldSectorRange = { 0 };
}

void c_CHARACTER::OnRelease()
{
	pSession = nullptr;
	dwSessionID = -1;

	//dwAction = dfPACKET_CS_MOVE_STOP;
	//byDirection = dfPACKET_MOVE_DIR_RR;
	//byMoveDirection = dfPACKET_MOVE_DIR_RR;
	////shX = (rand() % (dfRANGE_MOVE_RIGHT - dfRANGE_MOVE_LEFT + 1)) + dfRANGE_MOVE_LEFT;
	//shX = PlayerFirstX;
	////shY = (rand() % (dfRANGE_MOVE_BOTTOM - dfRANGE_MOVE_TOP + 1)) + dfRANGE_MOVE_TOP;
	//shY = PlayerFirstY;
	//chHP = PlayerFirstHP;

	//CurSector.iX = shX / dfSECTOR_X_Length;
	//CurSector.iY = shY / dfSECTOR_Y_Length;
	//CurSector.index = CurSector.iY * dfSECTOR_MAPMAX_X + CurSector.iX;

	//UpdateCurSectorRange();

	//OldSector.iX = -1;
	//OldSector.iY = -1;
	//OldSector.index = -1;

	//OldSectorRange = { 0 };
}

int c_CHARACTER::GetUpdateCurSectorIndex()
{
	CurSector.iX = shX / dfSECTOR_X_Length;
	CurSector.iY = shY / dfSECTOR_Y_Length;
	if (CurSector.iX < 0 || CurSector.iX > dfSECTOR_MAPMAX_X ||
		CurSector.iY < 0 || CurSector.iX > dfSECTOR_MAPMAX_Y)
	{
		while (1)
		{
			printf("[GetUpdateCurSectorIndex] 섹터 벗어남 감지\n");
		}
	}
	CurSector.index = CurSector.iY * dfSECTOR_MAPMAX_X + CurSector.iX;
	return CurSector.index;
}

int c_CHARACTER::UpdateCurSectorRange()
{
	OldSectorRange = CurSectorRange;

	int dx[9] = { -1,0,1,-1,0,1 ,-1,0,1 };
	int dy[9] = { -1,-1,-1,0,0,0,1,1,1 };

	for (int i = 0; i < 9; i++)
	{
		int nx = CurSector.iX + dx[i];
		int ny = CurSector.iY + dy[i];
		if (nx < 0 || nx >= dfSECTOR_MAPMAX_X ||
			ny < 0 || ny >= dfSECTOR_MAPMAX_Y)
		{
			CurSectorRange.Around[i].index = -1;
			continue;
		}
		CurSectorRange.Around[i].iX = nx;
		CurSectorRange.Around[i].iY = ny;
		CurSectorRange.Around[i].index = ny * dfSECTOR_MAPMAX_X + nx;
	}

	return 0;
}
