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