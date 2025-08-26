#include "Enemy.h"
#include "CScreenBuffer.h"
#include "CObjectManager.h"

//--------------------------------------------------------------------
// 자동으로 적 위치 좌표 이동
// 적들이 단체로 좌우 움직임 반복
// 가장 오른쪽 적과 왼쪽 적의 x좌표를 기준으로 움직임의 방향 바꿔준다.
//--------------------------------------------------------------------
void CEnemy :: MoveEnemys(CEnemy E[])
{
	//for (int i = 0; i < MAXENEMYNUM; i++)
	//{
	//	if (E[i]._Active)
	//	{
	//		E[i].MoveEnemy();
	//	}

	//	//MoveEnemy(&E[i]);
	//}
}

//--------------------------------------------------------------------
// 자동으로 적 위치 좌표 이동
// 적들이 각각 패턴을 기반으로 움직이도록 해준다.
//--------------------------------------------------------------------
void CEnemy::MoveEnemy()
{
	int Ecurstep = _pattern.curStep;
	int ex = _X;
	int ey = _Y;
	int dx = _pattern.Steps[Ecurstep].x;
	int dy = _pattern.Steps[Ecurstep].y;

	//적의 이전 위치 정보 저장
	_Prev_x = ex;
	_Prev_y = ey;

	if (ex + dx >= 0 && ex + dx < dfSCREEN_WIDTH - 1)
	{
		_X += dx;
	}
	if (ey + dy >= 0 && ey + dy < dfSCREEN_HEIGHT)
	{
		_Y += dy;
	}

	_pattern.curStep++;
	if (_pattern.curStep >= _pattern.stepCount)
	{
		_pattern.curStep = 0;
	}
}



//--------------------------------------------------------------------
// 적 총알 발사
//--------------------------------------------------------------------
void CEnemy::EnemysFire(CEnemy* ep, CBullet* bp)
{
	//CBullet* tb;
	//for (int i = 0; i < MAXENEMYNUM; i++)
	//{
	//	if (ep[i]._Active && (rand() % 100 < ep[i]._firePassability))
	//	{
	//		tb = CBullet::FindBullet();
	//		if (!tb)
	//		{
	//			printf("남은 총알 없음\n");
	//			return;
	//		}
	//		tb->InitBullet(true, ep[i]._X, ep[i]._Y, 1);
	//	}
	//}

}

void CEnemy::EnemyFire()
{
	CBullet* tb;
	if (_Active && (rand() % 100 < _firePassability))
	{
		tb = CBullet::FindBullet();
		if (!tb)
		{
			printf("남은 총알 없음\n");
			return;
		}
		tb->InitBullet(true, _X, _Y, 1);

		CObjectManager* CurObjManager = CObjectManager::GetInstance();
		CurObjManager->CreateObject(tb);
	}
}

//--------------------------------------------------------------------
// 적 파일 데이터 로드
//--------------------------------------------------------------------
bool CEnemy::LoadEnemys(CEnemy e[])
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

bool CEnemy::LoadEnemy(CEnemy* e, const char* filename)
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

	e->_X = fx;
	e->_Y = fy;
	e->_shape = fshape;
	e->_directionX = fdirectionX;
	e->_hp = fhp;
	e->_firePassability = ffirePassability;
	e->_Active = (bool)fbActive;

	bool findpattern = false;
	//이부분 함수로 만들어서 MovePattern쪽에 넣을까?
	for (int i = 0; i < MAXPATTERNTYPENUM; i++)
	{
		if (strcmp(PatternType[i].name, patternName) == 0)
		{
			fp = PatternType[i];
			e->_pattern = fp;
			return true;
		}
	}
	printf("적 값 못 읽음.\n");
	return false;
}


//--------------------------------------------------------------------
// 적이 총알 데미지 입음
//--------------------------------------------------------------------
void CEnemy::CheckDamagedEnemys(CBullet* bp, CEnemy* ep)
{
	for (int i = 0; i < MAXBULLETNUM; i++)
	{
		if (!bp[i].IsAvailable() || bp[i].FromEnemy()) continue;
		for (int j = 0; j < MAXENEMYNUM; j++)
		{
			if (!ep[j]._Active) continue;
			if(CheckBulletCollision(&bp[i], &ep[j]))
			{
				bp[i].Deactivate();
				ep[j]._hp -= 1;
				if (ep[j]._hp <= 0)
				{
					ep[j]._Active = false;
				}
			}
		}
	}
}

void CEnemy::CheckDamagedEnemy(CBaseObject* other)
{
	if (other->GetObjectType() == BULLET)
	{
		CBullet* bp = (CBullet*)other;
		if (!bp->FromEnemy())
		{
			_hp -= 1;
			if (_hp <= 0 && _Active)
			{
				_Active = false;
			}
		}
	}
}

bool CEnemy::CheckBulletCollision(CBullet* bp, CEnemy* ep)
{
	bool collisionCheck = true;

	do {
		//적과 총알의 위치 일치
		if (bp->Getpos_X() == ep->_X && bp->Getpos_Y() == ep->_Y) break;

		//적의 이전 위치와 총알의 현재 위치 일치
		if (bp->Getpos_X() == ep->_Prev_x && bp->Getpos_Y() == ep->_Prev_y) break;

		collisionCheck = false;
	
	} while (0);

	return collisionCheck;
}

//update()
//Enemy : EnemyFire, MoveEnemys, CheckDamagedEnemy

bool CEnemy::Update(void)
{
	MoveEnemy();
	EnemyFire();

	return true;
}

void CEnemy::OnCollision(CBaseObject* other)
{
	CheckDamagedEnemy(other);
}

void CEnemy::Render(void)
{
	CScreenBuffer* CurScreen = CScreenBuffer::GetInstance();
	if (IsAvailable())
	{
		CurScreen->Sprite_Draw(_X,_Y,_shape);
	}
}
