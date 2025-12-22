#include "Character.h"
#include "Session.h"
#include "Protocol.h"

c_CHARACTER::c_CHARACTER(SOCKETINFO* psession, DWORD sessionID)
{
	pSession = psession;
	dwSessionID = sessionID;

	dwAction = dfPACKET_CS_MOVE_STOP;
	byDirection = dfPACKET_MOVE_DIR_RR;
	byMoveDirection = byDirection;
	shX = (rand() % (dfRANGE_MOVE_RIGHT - dfRANGE_MOVE_LEFT + 1)) + dfRANGE_MOVE_LEFT;
	shY = (rand() % (dfRANGE_MOVE_BOTTOM - dfRANGE_MOVE_TOP + 1)) + dfRANGE_MOVE_TOP;
	chHP = 100;

	CurSector.iX = shX / dfSECTOR_MAX_X;
	CurSector.iY = shY / dfSECTOR_MAX_Y;

	OldSector = CurSector;
}

c_CHARACTER::~c_CHARACTER()
{
}
