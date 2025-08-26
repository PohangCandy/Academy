#include "Player.h"
#include "CScreenBuffer.h"
#include "CObjectManager.h"

//--------------------------------------------------------------------
// 키 입력에 따라 플레이어의 위치 좌표 이동
//--------------------------------------------------------------------
void CPlayer ::MovePlayer()
{
	int dx = 0;
	int dy = 0;

	if (GetAsyncKeyState(VK_LEFT) & 0x8001)
	{
		dx = -1;
	}
	if (GetAsyncKeyState(VK_RIGHT) & 0x8001)
	{
		dx = 1;
	}
	if (GetAsyncKeyState(VK_UP) & 0x8001)
	{
		dy = -1;
	}
	if (GetAsyncKeyState(VK_DOWN) & 0x8001)
	{
		dy = 1;
	}
	if (GetAsyncKeyState(VK_SPACE) & 0x8001)
	{
		if (_Active)
		{
			PlayerFire();
		}
	}

	int nx = _X + dx;
	int ny = _Y + dy;

	//if (p->Active && nx >= 0 && nx < dfSCREEN_WIDTH - 1 && ny >= 0 && ny < dfSCREEN_HEIGHT)
	//{
	//	p->x = p->x + dx;
	//	p->y = p->y + dy;
	//}
	if (_Active && nx >= 0 && nx < dfSCREEN_WIDTH - 1)
	{
		_X = _X + dx;
	}	
	if (ny >= 0 && ny < dfSCREEN_HEIGHT)
	{
		_Y = _Y + dy;
	}
}

//--------------------------------------------------------------------
// 플레이어 파일 데이터 로드
//--------------------------------------------------------------------
bool CPlayer::LoadPlayer(CPlayer* p)
{
	CParser Parser;
	int fx = 0;
	int fy = 0;
	char fshape = '@';
	int fhp = 0;
	int fbActive = 0;

	bool bSuccess = false;

	if (!Parser.LoadFile("Player.txt"))
	{
		printf("플레이어 파일 로딩 실패\n");
		return false;
	};

	do {
		if (!Parser.GetValue("PlayerXPos", &fx))
		{
			break;
		}
		if (!Parser.GetValue("PlayerYPos", &fy))
		{
			break;
		}
		if (!Parser.GetCharacter("PlayerShape", &fshape))
		{
			break;
		}
		if (!Parser.GetValue("PlayerHp", &fhp))
		{
			break;
		}
		if (!Parser.GetValue("PlayerActive", &fbActive))
		{
			break;
		}

		bSuccess = true;
	} while (0);

	if (!bSuccess)
	{
		printf("데이터 값 로딩 실패\n");
		return false;;
	}

	if (fx < 0 || fx >= dfSCREEN_WIDTH || fy < 0 || fy >= dfSCREEN_HEIGHT)
	{
		printf("잘못된 플레이어 위치\n");
		return false;
	}

	p->_X = fx;
	p->_Y = fy;
	p->_shape = fshape;
	p->_hp = fhp;
	p->_Active = (bool)fbActive;
	return true;
}


//--------------------------------------------------------------------
// 플레이어 사망 체크
//--------------------------------------------------------------------
bool CPlayer::CheckGameOver(CPlayer* p)
{
	if (p->_Active == false)
	{
		return true;
	}
	return false;
}

//--------------------------------------------------------------------
// 플레이어 총알 발사
// 
// 총알을 플레이어 위치에 생성
//--------------------------------------------------------------------
void CPlayer::PlayerFire()
{
	CBullet* tb = CBullet::FindBullet();
	if (!tb)
	{
		printf("남은 총알 없음\n");
		return;
	}
	tb->InitBullet(false, _X, _Y, -1);

	CObjectManager* CurObjManager = CObjectManager::GetInstance();
	CurObjManager->CreateObject(tb);
}

//--------------------------------------------------------------------
// 플레이어와 총알 충돌 체크
//--------------------------------------------------------------------
void CPlayer::CheckPlayerHit(CBullet* bp, CPlayer* p)
{
	for (int i = 0; i < MAXBULLETNUM; i++)
	{
		if (!bp[i].IsAvailable() || !bp[i].FromEnemy()) continue;

		if (!p->_Active) return;

		if (p->_X == bp[i].Getpos_X() && p->_Y == bp[i].Getpos_Y())
		{
			bp[i].Deactivate();
			p->_hp -= 1;
			if (p->_hp <= 0)
			{
				p->_Active = false;
			}
		}
	}
}


//update()
//Player: MovePlayer, CheckPlayerHit, CkeckGameOver

bool CPlayer::Update(void)
{
	MovePlayer();

	return true;
}

void CPlayer::OnCollision(CBaseObject* other)
{
	//총알과 부딫치면 충돌처리
	if (other->GetObjectType() == BULLET)
	{
		CBullet* bp = (CBullet*)other;
		if (bp->FromEnemy())
		{
			_hp -= 1;
			if (_hp <= 0)
			{
				_Active = false;
			}
		}
	}
}


void CPlayer::Render(void)
{
	CScreenBuffer* CurScreen = CScreenBuffer::GetInstance();
	if (IsAvailable())
	{
		CurScreen->Sprite_Draw(_X, _Y, _shape);
	}
}
