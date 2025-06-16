#include "Enemy.h"

//--------------------------------------------------------------------
// 자동으로 적 위치 좌표 이동
// 적들이 단체로 좌우 움직임 반복
// 가장 오른쪽 적과 왼쪽 적의 x좌표를 기준으로 움직임의 방향 바꿔준다.
//--------------------------------------------------------------------
void MoveEnemys(tag_Enemy E[])
{
	for (int i = 0; i < MAXENEMYNUM; i++)
	{
		if (E[i].Active)
		{
			MoveEnemy(&E[i]);
		}

		//MoveEnemy(&E[i]);
	}
}

//--------------------------------------------------------------------
// 자동으로 적 위치 좌표 이동
// 적들이 각각 패턴을 기반으로 움직이도록 해준다.
//--------------------------------------------------------------------
void MoveEnemy(tag_Enemy* E)
{
	int Ecurstep = E->pattern.curStep;
	int ex = E->x;
	int ey = E->y;
	int dx = E->pattern.Steps[Ecurstep].x;
	int dy = E->pattern.Steps[Ecurstep].y;

	if (ex + dx >= 0 && ex + dx < dfSCREEN_WIDTH - 1)
	{
		E->x += dx;
	}
	if (ey + dy >= 0 && ey + dy < dfSCREEN_HEIGHT)
	{
		E->y += dy;
	}

	E->pattern.curStep++;
	if (E->pattern.curStep >= E->pattern.stepCount)
	{
		E->pattern.curStep = 0;
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
			tb->shape = tb->eshape;
			tb->x = ep[i].x;
			tb->y = ep[i].y;
			tb->directionY = 1;
		}
	}

}

//--------------------------------------------------------------------
// 적 파일 데이터 로드
//--------------------------------------------------------------------
bool LoadEnemys(tag_Enemy e[])
{
	CParser Parser;
	int typeNum = 0;

	bool bSuccess = false;

	if (!Parser.LoadFile("EnemyInfom.txt"))
	{
		printf("적 파일 로딩 실패\n");
	};

	do {
		if (!Parser.GetValue("EnemyTypeNum", &typeNum))
		{
			break;
		}

		bSuccess = true;
	} while (0);

	if (!bSuccess)
	{
		printf("적 데이터 값 로딩 실패\n");
		return false;
	}

	for (int i = 0; i < typeNum; i++)
	{
		char *buff,p[256];
		char filename[256];
		int ilength;
		if(!Parser.RemoveSpace()) return false;
		Parser.GetNextWord(&buff, &ilength);
		memset(p, 0, 256);
		memcpy(p, buff, ilength);
		sprintf_s(filename, "%s.txt", p);
		LoadEnemy(&EnemyType[i],filename);
	}
	return true;
}

bool LoadEnemy(tag_Enemy* e, const char* filename)
{

	CParser Parser;
	char fshape = '@';
	int fx = 0;
	int fy = 0;
	int fdirectionX = 0;
	int fhp = 0;
	int ffirePassability = 0;
	int fbActive = 0;
	char patternName[12] = { 0, };
	tag_Pattern fp;

	bool bSuccess = false;

	if (!Parser.LoadFile(filename))
	{
		printf("파일 로딩 실패\n");
	};

	do {
		if (!Parser.GetValue("EnemyXpos", &fx))
		{
			break;
		}
		if (!Parser.GetValue("EnemyYpos", &fy))
		{
			break;
		}
		if (!Parser.GetCharacter("Enemyshape", &fshape))
		{
			break;
		}
		if (!Parser.GetValue("EnemydirectionX", &fdirectionX))
		{
			break;
		}
		if (!Parser.GetValue("Enemyhp", &fhp))
		{
			break;
		}
		if (!Parser.GetValue("EnemyfirePassability", &ffirePassability))
		{
			break;
		}
		if (!Parser.GetValue("EnemyActive", &fbActive))
		{
			break;
		}
		if (!Parser.GetCharacter("EnemyPattern", patternName))
		{
			break;
		}

		bSuccess = true;
	} while (0);

	if (!bSuccess)
	{
		printf("적 데이터 값 로딩 실패\n");
		return false;
	}

	if (fx < 0 || fx >= dfSCREEN_WIDTH || fy < 0 || fy >= dfSCREEN_HEIGHT)
	{
		printf("잘못된 적 위치\n");
		return false;
	}

	e->x = fx;
	e->y = fy;
	e->shape = fshape;
	e->directionX = fdirectionX;
	e->hp = fhp;
	e->firePassability = ffirePassability;
	e->Active = (bool)fbActive;

	bool findpattern = false;
	//이부분 함수로 만들어서 MovePattern쪽에 넣을까?
	for (int i = 0; i < MAXPATTERNTYPENUM; i++)
	{
		if (strcmp(PatternType[i].name, patternName) == 0)
		{
			fp = PatternType[i];
			e->pattern = fp;
			return true;
		}
	}
	printf("적 값 못 읽음.\n");
	return false;
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