#include "Bullet.h"

//--------------------------------------------------------------------
// 총알 모양 파일에서 데이터 읽기
//--------------------------------------------------------------------
bool loadBullet(tag_Bullet b[])
{
	static CParser Parser;

	char ftype[12] = "none";
	bool fActive = false;

	char fshape = 'O';
	char fpshape = 'O';
	char feshape = 'O';
	int fx = 0;
	int fy = 0;
	bool fbEnemy = false;
	int fdirectionY = -1;

	bool bSuccess = false;

	if (!Parser.LoadFile("Bullet.txt"))
	{
		printf("총알 파일 로딩 실패\n");
		return false;
	};

	do {
		if (!Parser.GetCharacter("type", ftype))
		{
			break;
		}
		if (!Parser.GetCharacter("pShape", &fpshape))
		{
			break;
		}
		if (!Parser.GetCharacter("eShape", &feshape))
		{
			break;
		}

		bSuccess = true;
	} while (0);

	if (!bSuccess)
	{
		printf("총알 데이터 값 로딩 실패\n");
		return false;;
	}

	for (int i = 0; i < MAXBULLETTYPE; i++)
	{
		strcpy(b[i].type, ftype);
		b[i].Active = fActive;
		b[i].shape = fshape;
		b[i].pshape = fpshape;
		b[i].eshape = feshape;
		b[i].x = fx;
		b[i].y = fy;
		b[i].bEnemy = fbEnemy;
		b[i].directionY = fdirectionY;
	}
	return true;
}

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
				//bp[i].shape = 'x';
				/*for (int i = 0; i < MAXBULLETTYPE; i++)
				{
					if (strcmp(bp[i].type, BulletType[i].type) == 0)
					{
						bp[i].shape = BulletType[i].eshape;
						break;
					}
				}*/
				//bp[i].directionY = 1;
			}
			else
			{
				//bp[i].shape = 'o';
				/*for (int i = 0; i < MAXBULLETTYPE; i++)
				{
					if (strcmp(bp[i].type, BulletType[i].type) == 0)
					{
						bp[i].shape = BulletType[i].pshape;
						break;
					}
				}*/
				//bp[i].directionY = -1;
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