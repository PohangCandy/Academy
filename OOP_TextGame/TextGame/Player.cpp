#include "Player.h"

//--------------------------------------------------------------------
// 키 입력에 따라 플레이어의 위치 좌표 이동
//--------------------------------------------------------------------
void CPlayer ::MovePlayer(CPlayer* p, CBullet* bp)
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
		if (p->_Active)
		{
			PlayerFire(p, bp);
		}
	}

	int nx = p->_X + dx;
	int ny = p->_Y + dy;

	//if (p->Active && nx >= 0 && nx < dfSCREEN_WIDTH - 1 && ny >= 0 && ny < dfSCREEN_HEIGHT)
	//{
	//	p->x = p->x + dx;
	//	p->y = p->y + dy;
	//}
	if (p->_Active && nx >= 0 && nx < dfSCREEN_WIDTH - 1)
	{
		p->_X = p->_X + dx;
	}	
	if (ny >= 0 && ny < dfSCREEN_HEIGHT)
	{
		p->_Y = p->_Y + dy;
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
bool CPlayer::CkeckGameOver(CPlayer* p)
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
void CPlayer::PlayerFire(CPlayer* p, CBullet* bp)
{
	CBullet* tb = FindBullet(bp);
	if (!tb)
	{
		printf("남은 총알 없음\n");
		return;
	}
	tb->InitBullet(false, p->_X, p->_Y, -1);
}

//--------------------------------------------------------------------
// 플레이어와 총알 충돌 체크
//--------------------------------------------------------------------
void CPlayer::CheckPlayerHit(CBullet* bp, CPlayer* p)
{
	for (int i = 0; i < MAXBULLETNUM; i++)
	{
		if (!bp[i]._Active || !bp[i]._bEnemy) continue;

		if (!p->_Active) return;

		if (p->_X == bp[i]._X && p->_Y == bp[i]._Y)
		{
			bp[i]._Active = false;
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

	return false;
}

void CPlayer::Render(void)
{

}

void CPlayer::OnCollision(CBaseObject* other)
{
	//총알과 부딫치면 충돌처리
	if (other->GetObjectType() == 2)
	{
		//적이 쏜 총알인지 플레이어가 쏜 총알인지 확인 필요
		//총알 종류이니 BaseObject 타입으로 넣을게 아니라, 총알 종류로 넣어줘야 함.
		// CBullet* bp = dynamic_cast<CBullet*>(other);
		//다운 캐스팅으로 총알을 받아야하나..근데 이럴거면 타입을 인자로 넣은 의미가 없는데
		//걍 총알 타입이니까 믿고 변환한 다음에 맴버 호출해버릴까?
		CBullet* bp = (CBullet*)other;
		//적 총알이면 플레이어에게 충돌 처리를 해준다.
		if (bp->bEnemy)
		{

		}
	}
}
