#include "Bullet.h"

//--------------------------------------------------------------------
// 자동으로 총알 위치 좌표 이동
// 종류에 따라 위 or 아래로 움직임
// 플레이어는 스페이스로 총알 생성
// 적은 랜덤한 시간으로 총알 생성
//--------------------------------------------------------------------
void MoveBullet(tag_Bullet* bp)
{
	for (int i = 0; i < MAXBULLETNUM; i++)
	{
		if (bp[i].Active)
		{
			if (bp[i].bEnemy)
			{
				bp[i].shape = 'x';
				bp[i].directionY = 1;
			}
			else
			{
				bp[i].shape = 'o';
				bp[i].directionY = -1;
			}

			int y = bp[i].y;
			int dy = bp[i].directionY;
			if (y + dy < 0 || y + dy > dfSCREEN_HEIGHT - 1)
			{
				bp[i].Active = false;
			}
			else
			{
				bp[i].y += dy;
			}
		}
	}
}

//--------------------------------------------------------------------
// 총알 메모리 풀에서 사용가능한 총알 반환
//--------------------------------------------------------------------
tag_Bullet* FindBullet(tag_Bullet* bp)
{
	for (int i = 0; i < MAXBULLETNUM; i++)
	{
		if (!(bp[i].Active))
		{
			bp[i].Active = true;
			return &bp[i];
		}
	}

	return nullptr;
}