#include "Enemy.h"

//--------------------------------------------------------------------
// 자동으로 적 위치 좌표 이동
// 적들이 단체로 좌우 움직임 반복
// 가장 오른쪽 적과 왼쪽 적의 x좌표를 기준으로 움직임의 방향 바꿔준다.
//--------------------------------------------------------------------
void MoveEnemy(tag_Enemy* E)
{
	//우로 이동
	if (E->directionX == 1)
	{
		//dfSCREEN_WIDTH에 \n 들어가므로 dfSCREEN_WIDTH - 1까지만 이동하게 만든다. 
		if (E[MAXENEMYNUM - 1].x + 1 >= dfSCREEN_WIDTH - 1)
		{
			E->directionX = -1;
		}
	}
	//좌로 이동
	else
	{
		if (E[0].x - 1 < 0)
		{
			E->directionX = 1;
		}
	}

	int nx = E->directionX;

	for (int i = 0; i < MAXENEMYNUM; i++)
	{
		E[i].x += nx;
	}
}



//--------------------------------------------------------------------
// 적 총알 발사
//--------------------------------------------------------------------
void EnemyFire(tag_Enemy* ep, tag_Bullet* bp)
{
	tag_Bullet* tb;
	for (int i = 0; i < MAXENEMYNUM; i++)
	{
		if (ep[i].Active && (rand() % 100 < ep[i].firePassability))
		{
			tb = FindBullet(bp);
			if (!tb)
			{
				printf("남은 총알 없음\n");
				return;
			}
			tb->bEnemy = true;
			tb->x = ep[i].x;
			tb->y = ep[i].y;
		}
	}

}

void LoadEnemy(tag_Enemy* e)
{

	static CParser Parser;
	char fshape = '@';
	int fx = 0;
	int fy = 0;
	int fdirectionX = 0;
	int fhp = 0;
	int ffirePassability = 0;
	int fbActive = 0;

	bool bSuccess = false;

	if (!Parser.LoadFile("EnemyA.txt"))
	{
		printf("파일 로딩 실패\n");
	};

	do {
		if (!Parser.GetValue("EnemyAXpos", &fx))
		{
			break;
		}
		if (!Parser.GetValue("EnemyAYpos", &fy))
		{
			break;
		}
		if (!Parser.GetCharacter("EnemyAshape", &fshape))
		{
			break;
		}
		if (!Parser.GetValue("EnemyAdirectionX", &fdirectionX))
		{
			break;
		}
		if (!Parser.GetValue("EnemyAhp", &fhp))
		{
			break;
		}
		if (!Parser.GetValue("EnemyAfirePassability", &ffirePassability))
		{
			break;
		}
		if (!Parser.GetValue("EnemyAActive", &fbActive))
		{
			break;
		}

		bSuccess = true;
	} while (0);

	if (!bSuccess)
	{
		printf("데이터 값 로딩 실패\n");
		return;
	}

	if (fx < 0 || fx >= dfSCREEN_WIDTH || fy < 0 || fy >= dfSCREEN_HEIGHT)
	{
		printf("잘못된 플레이어 위치\n");
		return;
	}

	e->x = fx;
	e->y = fy;
	e->shape = fshape;
	e->directionX = fdirectionX;
	e->hp = fhp;
	e->firePassability = ffirePassability;
	e->Active = (bool)fbActive;
}


//--------------------------------------------------------------------
// 총알 충돌 체크
//--------------------------------------------------------------------
void CheckBulletCollision(tag_Bullet* bp, tag_Enemy* ep)
{
	for (int i = 0; i < MAXBULLETNUM; i++)
	{
		if (!bp[i].Active || bp[i].bEnemy) continue;
		for (int j = 0; j < MAXENEMYNUM; j++)
		{
			if (!ep[j].Active) continue;

			if (bp[i].x == ep[j].x && bp[i].y == ep[j].y)
			{
				bp[i].Active = false;
				ep[j].hp -= 1;
				if (ep[j].hp <= 0)
				{
					ep[j].Active = false;
				}
			}
		}
	}
}