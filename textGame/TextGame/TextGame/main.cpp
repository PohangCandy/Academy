#include <stdio.h>
#include <memory.h>
#include <Windows.h>
#include "Console.h"
#include "TextParser.h"

/*
* 
*  이 코드는 샘플이며 이 코드를 유지하며 만드실 필요가 없습니다.
*  참고하여 마음대로 만드십쇼.
* 
*/

//--------------------------------------------------------------------
//적 최대 수
//--------------------------------------------------------------------
#define MAXENEMYNUM 10

//--------------------------------------------------------------------
//총알 최대 수
//--------------------------------------------------------------------
#define MAXBULLETNUM 100

//--------------------------------------------------------------------
// 화면 깜빡임을 없애기 위한 화면 버퍼.
// 게임이 진행되는 상황을 매번 화면을 지우고 비행기 찍고, 지우고 찍고,
// 하게 되면 화면이 깜빡깜빡 거리게 된다.
//
// 그러므로 화면과 똑같은 크기의 메모리를 할당한 다음에 화면에 바로 찍지않고
// 메모리(버퍼)상에 그림을 그리고 메모리의 화면을 그대로 화면에 찍어준다.
//
// 이렇게 해서 화면을 매번 지우고, 그리고, 지우고, 그리고 하지 않고
// 메모리(버퍼)상의 그림을 화면에 그리는 작업만 하게 되어 깜박임이 없어진다.
//
// 버퍼의 각 줄 마지막엔 NULL 을 넣어 문자열로서 처리하며, 
// 한줄한줄을 printf 로 찍어나갈 것이다.
//
// for ( N = 0 ~ height )
// {
// 	  cs_MoveCursor(0, N);
//    printf(szScreenBuffer[N]);
// }
//
// 줄바꿈에 printf("\n") 을 쓰지 않고 커서좌표를 이동하는 이유는
// 화면을 꽉 차게 출력하고 줄바꿈을 하면 2칸이 내려가거나 화면이 밀릴 수 있으므로
// 매 줄 출력마다 좌표를 강제로 이동하여 확실하게 출력한다.
//--------------------------------------------------------------------
char szScreenBuffer[dfSCREEN_HEIGHT][dfSCREEN_WIDTH];


//--------------------------------------------------------------------
// GetAsyncKeyState(int iKey)  #include <Windows.h>
//
// 윈도우 API 로 키보드가 눌렸는지를 확인한다.
// 인자로 키보드 버튼에 대한 디파인 값을 넣으면 해당 키가 눌렸는지 (눌렸던적이 있는지) 를 확인 해준다.
// 모든 키에대한 확인이 가능하고, 논블럭 체크가 되므로 게임에서도 쓰기 좋다.
//
// Virtual-Key Codes
//
// VK_SPACE / VK_ESCAPE / VK_LEFT / VK_UP / 키보드 문자는 대문자 아스키 코드와 같음.
// winuser.h 파일에 위와 같이 디파인 되어 있다.
//
//
// GetAsyncKeyState(VK_LEFT) 호출시 결과값은
//
// 0x0001  > *이전 체크 이후 눌린적이 있음
// 0x8000  > 지금 눌려있음
// 0x8001  > *이전 체크 이후 눌린적도 있고 지금도 눌려 있음
//
// * 이전 체크라는건 이전에 GetAsyncKeyState 를 호출한 때를 말 한다.
// ------------------Q. 체크 간격 100ms아님?
// 10프레임 짜리 게임이라면 1초에 10회의 키 체크를 하게 되므로 체크 간격은 20ms 가 된다.
// 빠른 커맨드 입력이 필요한 게임에서는 20ms 이내에 여러개의 키입력이 있다면 체크하지 못하는 키 입력이 발생 할 수 있다.
// 그래서 0x0001 비트에 대한 처리도 필요하다.
//


//--------------------------------------------------------------------
// 버퍼의 내용을 화면으로 찍어주는 함수.
//
// 적군,아군,총알 등을 szScreenBuffer 에 넣어주고, 
// 1 프레임이 끝나는 마지막에 본 함수를 호출하여 버퍼 -> 화면 으로 그린다.
//--------------------------------------------------------------------
void Buffer_Flip(void);
//--------------------------------------------------------------------
// 화면 버퍼를 지워주는 함수
//
// 매 프레임 그림을 그리기 직전에 버퍼를 지워 준다. 
// 안그러면 이전 프레임의 잔상이 남으니까
//--------------------------------------------------------------------
void Buffer_Clear(void);

//--------------------------------------------------------------------
// 버퍼의 특정 위치에 원하는 문자를 출력.
//
// 입력 받은 X,Y 좌표에 아스키코드 하나를 출력한다. (버퍼에 그림)
//--------------------------------------------------------------------
void Sprite_Draw(int iX, int iY, char chSprite);

//--------------------------------------------------------------------
// 플레이어의 정보
// 
// 플레이어의 모양
// 플레이어의 시작 위치
//--------------------------------------------------------------------
struct tag_Player
{
	char shape = '@';
	int x = 0;
	int y = 0;
	int hp = 0;
	bool Active = 0;
};

//--------------------------------------------------------------------
// 적의 정보
// 
// 적의 모양
// 적의 시작 위치
//--------------------------------------------------------------------
struct tag_Enemy
{
	char shape = '@';
	int x = 0;
	int y = 0;
	int directionX = 1;
	int hp = 3;
	int firePassability = 50;
	bool Active = 0;
};
tag_Enemy EP[MAXENEMYNUM];

//--------------------------------------------------------------------
// 총알의 정보
// 
// 총알의 모양
// 총알의 위치
// 총알 종류
// 총알 방향
//--------------------------------------------------------------------
struct tag_Bullet
{
	bool Active = false;

	char shape = 'O';
	int x = 0;
	int y = 0;
	bool bEnemy = false;
	int directionY = -1;
};

//--------------------------------------------------------------------
//총알 메모리풀
//--------------------------------------------------------------------
tag_Bullet BP[MAXBULLETNUM];


//--------------------------------------------------------------------
// 키 입력에 따라 플레이어의 위치 좌표 이동
//--------------------------------------------------------------------
void MovePlayer(tag_Player* p, tag_Bullet* bp);

//--------------------------------------------------------------------
// 자동으로 적 위치 좌표 이동
// 적들이 단체로 좌우 움직임 반복
// 가장 오른쪽 적과 왼쪽 적의 x좌표를 기준으로 움직임의 방향 바꿔준다.
//--------------------------------------------------------------------
void MoveEnemy(tag_Enemy* E);

//--------------------------------------------------------------------
// 자동으로 총알 위치 좌표 이동
// 종류에 따라 위 or 아래로 움직임
// 플레이어는 스페이스로 총알 생성
// 적은 랜덤한 시간으로 총알 생성
//--------------------------------------------------------------------
void MoveBullet(tag_Bullet* bp);

//--------------------------------------------------------------------
// 총알 메모리 풀에서 사용가능한 총알 반환
//--------------------------------------------------------------------
tag_Bullet* FindBullet(tag_Bullet* bp);

//--------------------------------------------------------------------
// 적과 총알 충돌 체크
//--------------------------------------------------------------------
void CheckBulletCollision(tag_Bullet* bp, tag_Enemy* ep);

//--------------------------------------------------------------------
// 플레이어와 총알 충돌 체크
//--------------------------------------------------------------------
void CheckPlayerHit(tag_Bullet* bp, tag_Player* p);

//--------------------------------------------------------------------
// 플레이어 파일 데이터 로드
//--------------------------------------------------------------------
void LoadPlayer(tag_Player* p);

//--------------------------------------------------------------------
// 플레이어 총알 발사
//--------------------------------------------------------------------
void PlayerFire(tag_Player* p, tag_Bullet* bp);

//--------------------------------------------------------------------
// 적 총알 발사
//--------------------------------------------------------------------
void EnemyFire(tag_Enemy* ep, tag_Bullet* bp);



void main(void)
{
	cs_Initial();
	
	tag_Player P;
	LoadPlayer(&P);

	//적의 개수보다 하나 더 많이 나눠야 적이 화면 끝에 위치하지 않는다.	
	int divide = dfSCREEN_WIDTH / (MAXENEMYNUM + 1);

	for (int i = 0; i < MAXENEMYNUM; i++)
	{
		EP[i].shape = 'E';
		EP[i].y = 3;
		EP[i].x = divide * (i + 1);
		EP[i].hp = 3;
		EP[i].Active = 1;
	}

	//for (int i = 0; i < MAXBULLETNUM; i++)
	//{
	//	BP[i].Active = false;
	//	BP[i].shape = 'O';
	//	BP[i].bEnemy = false;
	//	BP[i].x = 0;
	//	BP[i].y = 0;
	//	BP[i].directionY = -1;
	//}

	//-------------------------------------------------------------------
	// 게임의 메인 루프
	// 이 루프가  1번 돌면 1프레임 이다.
	//--------------------------------------------------------------------
	while (1)
	{
		// 하단은 게임씬의 로직 예시이며 
		// 이 부분에는 씬 표현을 위한 분기가 들어가시면 됩니다.
		// 
		// 
		// GameUpdate() 내부 예시
		// 
		// 1. 키보드 입력부
		MovePlayer(&P,BP);
		EnemyFire(EP, BP);
		// 
		// 2. 로직부 
		MoveBullet(BP);
		MoveEnemy(EP);
		CheckBulletCollision(BP, EP);
		CheckPlayerHit(BP, &P);
		// 3. 랜더부
			  //예시
				// 스크린 버퍼를 지움
				Buffer_Clear();
				// 스크린 버퍼에 객체들 출력
				//DrawBullet
				for (int i = 0; i < MAXBULLETNUM; i++)
				{
					if (BP[i].Active)
					{
						Sprite_Draw(BP[i].x, BP[i].y, BP[i].shape);
					}
				}

				//DrawEnemy
				for (int i = 0; i < MAXENEMYNUM; i++)
				{
					if (EP[i].Active)
					{
						Sprite_Draw(EP[i].x, EP[i].y, EP[i].shape);
					}
				}

				//DrawPlayer
				if (P.Active)
				{
					Sprite_Draw(P.x, P.y, P.shape);
				}
				// 스크린 버퍼를 화면으로 출력
				Buffer_Flip();
			

		// 프레임 맞추기용 대기 Sleep(X)
				Sleep(100);
	}
}



//--------------------------------------------------------------------
// 버퍼의 내용을 화면으로 찍어주는 함수.
//
// 적군,아군,총알 등을 szScreenBuffer 에 넣어주고, 
// 1 프레임이 끝나는 마지막에 본 함수를 호출하여 버퍼 -> 화면 으로 그린다.
//--------------------------------------------------------------------
void Buffer_Flip(void)
{
	int iCnt;
	for (iCnt = 0; iCnt < dfSCREEN_HEIGHT; iCnt++)
	{
		cs_MoveCursor(0, iCnt);
		printf(szScreenBuffer[iCnt]);
	}
}


//--------------------------------------------------------------------
// 화면 버퍼를 지워주는 함수
//
// 매 프레임 그림을 그리기 직전에 버퍼를 지워 준다. 
// 안그러면 이전 프레임의 잔상이 남으니까
//--------------------------------------------------------------------
void Buffer_Clear(void)
{
	int iCnt;
	memset(szScreenBuffer, ' ', dfSCREEN_WIDTH * dfSCREEN_HEIGHT);

	for (iCnt = 0; iCnt < dfSCREEN_HEIGHT; iCnt++)
	{
		szScreenBuffer[iCnt][dfSCREEN_WIDTH - 1] = '\0';
	}

}

//--------------------------------------------------------------------
// 버퍼의 특정 위치에 원하는 문자를 출력.
//
// 입력 받은 X,Y 좌표에 아스키코드 하나를 출력한다. (버퍼에 그림)
//--------------------------------------------------------------------
void Sprite_Draw(int iX, int iY, char chSprite)
{
	if (iX < 0 || iY < 0 || iX >= dfSCREEN_WIDTH - 1 || iY >= dfSCREEN_HEIGHT)
		return;

	szScreenBuffer[iY][iX] = chSprite;
}


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

	if (p->Active  && nx >= 0 && nx < dfSCREEN_WIDTH - 1 && ny >= 0 && ny < dfSCREEN_HEIGHT)
	{
		p->x = p->x + dx;
		p->y = p->y + dy;
	}
}

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
		if (E[MAXENEMYNUM-1].x + 1 >= dfSCREEN_WIDTH - 1)
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

//--------------------------------------------------------------------
// 플레이어 파일 데이터 로드
//--------------------------------------------------------------------
void LoadPlayer(tag_Player* p)
{
	static CParser Parser;
	int fx = 0;
	int fy = 0;
	char fshape = '@';
	int fhp = 0;
	int fbActive = 0;

	bool bSuccess = false;

	if (!Parser.LoadFile("test.txt"))
	{
		printf("파일 로딩 실패\n");
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
		return;
	}

	if (fx < 0 || fx >= dfSCREEN_WIDTH || fy < 0 || fy >= dfSCREEN_HEIGHT)
	{
		printf("잘못된 플레이어 위치\n");
		return;
	}

	p->x = fx;
	p->y = fy;
	p->shape = fshape;
	p->hp = fhp;
	p->Active = (bool)fbActive;
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
	tb->x = p->x;
	tb->y = p->y;
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
