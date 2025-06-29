#include "Player.h"

//--------------------------------------------------------------------
// 키 입력에 따라 플레이어의 위치 좌표 이동
//--------------------------------------------------------------------
void MovePlayer(tag_Player* p, tag_Bullet* bp)
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
		if (p->Active)
		{
			PlayerFire(p, bp);
		}
	}

	int nx = p->x + dx;
	int ny = p->y + dy;

	//if (p->Active && nx >= 0 && nx < dfSCREEN_WIDTH - 1 && ny >= 0 && ny < dfSCREEN_HEIGHT)
	//{
	//	p->x = p->x + dx;
	//	p->y = p->y + dy;
	//}
	if (p->Active && nx >= 0 && nx < dfSCREEN_WIDTH - 1)
	{
		p->x = p->x + dx;
	}	
	if (ny >= 0 && ny < dfSCREEN_HEIGHT)
	{
		p->y = p->y + dy;
	}
}

//--------------------------------------------------------------------
// 플레이어 파일 데이터 로드
//--------------------------------------------------------------------
bool LoadPlayer(tag_Player* p)
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

	p->x = fx;
	p->y = fy;
	p->shape = fshape;
	p->hp = fhp;
	p->Active = (bool)fbActive;
	return true;
}


//--------------------------------------------------------------------
// 플레이어 사망 체크
//--------------------------------------------------------------------
bool CkeckGameOver(tag_Player* p)
{
	if (p->Active == false)
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
void PlayerFire(tag_Player* p, tag_Bullet* bp)
{
	tag_Bullet* tb = FindBullet(bp);
	if (!tb)
	{
		printf("남은 총알 없음\n");
		return;
	}

	tb->bEnemy = false;
	tb->shape = tb->pshape;
	tb->x = p->x;
	tb->y = p->y;
	tb->directionY = -1;
}

//--------------------------------------------------------------------
// 플레이어와 총알 충돌 체크
//--------------------------------------------------------------------
void CheckPlayerHit(tag_Bullet* bp, tag_Player* p)
{
	for (int i = 0; i < MAXBULLETNUM; i++)
	{
		if (!bp[i].Active || !bp[i].bEnemy) continue;

		if (!p->Active) return;

		if (p->x == bp[i].x && p->y == bp[i].y)
		{
			bp[i].Active = false;
			p->hp -= 1;
			if (p->hp <= 0)
			{
				p->Active = false;
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
