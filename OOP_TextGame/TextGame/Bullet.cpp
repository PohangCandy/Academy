#include "Bullet.h"
#include "CScreenBuffer.h"

//--------------------------------------------------------------------
// 총알 모양 파일에서 데이터 읽기
//--------------------------------------------------------------------
bool CBullet :: loadBullet(CBullet b[])
{
	CParser Parser;

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
		strcpy(b[i]._type, ftype);
		b[i]._Active = fActive;
		b[i]._shape = fshape;
		b[i]._pshape = fpshape;
		b[i]._eshape = feshape;
		b[i]._X = fx;
		b[i]._Y = fy;
		b[i]._bEnemy = fbEnemy;
		b[i]._directionY = fdirectionY;
	}
	return true;
}

void CBullet::InitBullet(bool bE, int x, int y, int direction)
{
	_bEnemy = bE;
	if (bE)
	{
		_shape = _eshape;
	}
	else
	{
		_shape = _pshape;
	}
	_X = x;
	_Y = y;
	_directionY = direction;
}

bool CBullet::IsAvailable(void)
{
	return _Active;
}

void CBullet::Activate(void)
{
	_Active = true;
}

void CBullet::Deactivate(void)
{
	_Active = false;
}

bool CBullet::FromEnemy(void)
{
	if (_bEnemy)
	{
		return true;
	}
	return false;
}

//--------------------------------------------------------------------
// 자동으로 총알 위치 좌표 이동
// 종류에 따라 위 or 아래로 움직임
// 플레이어는 스페이스로 총알 생성
// 적은 랜덤한 시간으로 총알 생성
//--------------------------------------------------------------------
void  CBullet::MoveBullets(CBullet* bp)
{
	for (int i = 0; i < MAXBULLETNUM; i++)
	{
		if (bp[i]._Active)
		{
			if (bp[i]._bEnemy)
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

			int y = bp[i]._Y;
			int dy = bp[i]._directionY;
			if (y + dy < 0 || y + dy > dfSCREEN_HEIGHT - 1)
			{
				bp[i]._Active = false;
			}
			else
			{
				bp[i]._Y += dy;
			}
		}
	}
}

void CBullet::MoveBullet()
{
	if (_Active)
	{
		int y = _Y;
		int dy = _directionY;
		if (y + dy < 0 || y + dy > dfSCREEN_HEIGHT - 1)
		{
			_Active = false;
		}
		else
		{
			_Y += dy;
		}
	}
}

//--------------------------------------------------------------------
// 총알 메모리 풀에서 사용가능한 총알 반환
//--------------------------------------------------------------------
CBullet* CBullet::FindBullet()
{
	for (int i = 0; i < MAXBULLETNUM; i++)
	{
		if (!(BP[i]._Active))
		{
			BP[i]._Active = true;
			return &BP[i];
		}
	}

	return nullptr;
}

//update()
//Bullet : MoveBullet

bool CBullet::Update(void)
{
	MoveBullet();
	return false;
}

void CBullet::OnCollision(CBaseObject* other)
{
	if (this->_bEnemy && other->GetObjectType() != ENEMY)
	{
		_Active = false;
	}
	if (!this->_bEnemy && other->GetObjectType() == ENEMY)
	{
		_Active = false;
	}
}

void CBullet::Render(void)
{
	CScreenBuffer* CurScreen = CScreenBuffer::GetInstance();
	if (IsAvailable())
	{
		CurScreen->Sprite_Draw(_X, _Y, _shape);
	}
}
