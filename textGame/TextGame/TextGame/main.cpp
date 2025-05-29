#include <stdio.h>
#include <memory.h>
#include <Windows.h>
#include "Console.h"
#include "TextParser.h"
#include "Player.h"
#include "Bullet.h"
#include "Enemy.h"
#include "MovePattern.h"
#include "Stage.h"


//1. 시간 측정으로 프레임이 떨어지지 않도록 만든다.
//2. 오류 없애기
// 
// 
//3. 플레이어도 스테이지 파일에서 정보를 불러오게 해준다.
//4. 총알도 모양 다르게 타입별로 파일 데이터로 불러올 수 있을 듯
//5. 플레이어 두명도 가능할 듯. 타입과 메모리풀 선언 

/*
* 
*  이 코드는 샘플이며 이 코드를 유지하며 만드실 필요가 없습니다.
*  참고하여 마음대로 만드십쇼.
* 
*/

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


////--------------------------------------------------------------------
////플레이어 타입
////--------------------------------------------------------------------
//tag_Player PlayerType[MAXPlAYERTYPE];

////--------------------------------------------------------------------
////플레이어 메모리풀
////--------------------------------------------------------------------
tag_Player curPlayer;

//--------------------------------------------------------------------
//적 타입
//--------------------------------------------------------------------
tag_Enemy EnemyType[MAXENEMYTYPE];
//--------------------------------------------------------------------
//적 메모리풀
//--------------------------------------------------------------------
tag_Enemy EP[MAXENEMYNUM];


//--------------------------------------------------------------------
//총알 메모리풀
//--------------------------------------------------------------------
tag_Bullet BP[MAXBULLETNUM];

//--------------------------------------------------------------------
//패턴 메모리풀
//--------------------------------------------------------------------
tag_Pattern PatternType[MAXPATTERNTYPENUM];

//--------------------------------------------------------------------
//스테이지 메모리풀
//--------------------------------------------------------------------
tag_Stage StagePool[MAXSTAGENUM];

//--------------------------------------------------------------------
//게임 씬
//--------------------------------------------------------------------
enum  e_Scene
{
	Load,
	Title,
	Game,
	GameOver
};

//--------------------------------------------------------------------
//현재 게임 씬과 다음에 올 게임 씬
//--------------------------------------------------------------------
e_Scene CurScene,nextScene;

//--------------------------------------------------------------------
//게임 씬의 현재 스테이지 인덱스
//--------------------------------------------------------------------
int CurStageIndex;

//--------------------------------------------------------------------
//텍스트 씬 구조체
//--------------------------------------------------------------------
struct TextScene {
	char message[256];
	int xpos;
	int ypos;
};
//--------------------------------------------------------------------
//타이틀 씬과 게임오버 씬 
//--------------------------------------------------------------------
TextScene curTitle, curGameOver;

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
// 타이틀 씬 업데이트 함수
//--------------------------------------------------------------------
void updateTitle();

//--------------------------------------------------------------------
// 게임 씬 업데이트 함수
//--------------------------------------------------------------------
void updateGame();

//--------------------------------------------------------------------
// 게임오버 씬 업데이트 함수
//--------------------------------------------------------------------
void updateGameOver();

//--------------------------------------------------------------------
// 로딩 씬 업데이트 함수
// 각 씬 진입 전에 씬에 필요한 정보 로딩
//--------------------------------------------------------------------
void updateLoading();

//--------------------------------------------------------------------
// 각 오브젝트 풀 정리
// 버퍼 정리 -> clearBuff()
// 메모리 정리 ->  GameReset()
//--------------------------------------------------------------------
void GameReset();


void main(void)
{
	cs_Initial();
	CurScene = Load;
	nextScene = Title;
	CurStageIndex = 0;

	//-------------------------------------------------------------------
	// 게임의 메인 루프
	// 이 루프가  1번 돌면 1프레임 이다.
	//--------------------------------------------------------------------
	while (1)
	{
		switch (CurScene)
		{
		case Title:
			updateTitle();
			break;
		case Load:
			updateLoading();
			break;
		case Game:
			updateGame();
			break;
		case GameOver:
			updateGameOver();
			break;
		}
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
// 게임 씬 업데이트 함수
//--------------------------------------------------------------------
void updateGame()
{
	// 1. 키보드 입력부
	MovePlayer(&curPlayer, BP);
	EnemyFire(EP, BP);
	// 
	// 2. 로직부 
	MoveBullet(BP);
	MoveEnemys(EP);
	CheckBulletCollision(BP, EP);
	if (Stage_End())
	{
		if (CurStageIndex + 1 < MAXSTAGENUM)
		{
			GameReset();
			CurStageIndex++;
			CurScene = Load;
			nextScene = Game;
			return;
		}
		else
		{
			GameReset();
			CurStageIndex = 0;
			CurScene = Load;
			nextScene = GameOver;
			return;
		}
	}
	CheckPlayerHit(BP, &curPlayer);
	if (CkeckGameOver(&curPlayer))
	{
		GameReset();
		CurStageIndex = 0;
		CurScene = Load;
		nextScene = GameOver;
		return;
	}

	// 3. 랜더부
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
	if (curPlayer.Active)
	{
		Sprite_Draw(curPlayer.x, curPlayer.y, curPlayer.shape);
	}
	// 스크린 버퍼를 화면으로 출력
	Buffer_Flip();
}

//--------------------------------------------------------------------
// 타이틀 씬 업데이트 함수
//--------------------------------------------------------------------
void updateTitle()
{
	//입력
	if (GetAsyncKeyState(VK_SPACE) & 0x8001)
	{
		nextScene = Game;
		CurScene = Load;
	}

	//랜더
	Buffer_Clear();
	for (int i = 0; i < strlen(curTitle.message); i++)
	{
		Sprite_Draw(curTitle.xpos + i, curTitle.ypos, curTitle.message[i]);
	}
	Buffer_Flip();
}

//--------------------------------------------------------------------
// 게임오버 씬 업데이트 함수
//--------------------------------------------------------------------
void updateGameOver()
{
	//입력
	if (GetAsyncKeyState(VK_SPACE) & 0x8001)
	{
		CurScene = Load;
		nextScene = Title;
	}

	//랜더
	Buffer_Clear();
	for (int i = 0; i < strlen(curGameOver.message); i++)
	{
		Sprite_Draw(curGameOver.xpos + i, curGameOver.ypos, curGameOver.message[i]);
	}
	Buffer_Flip();
}

//--------------------------------------------------------------------
// 로딩 씬 업데이트 함수
//--------------------------------------------------------------------
void updateLoading()
{
	if (nextScene == Title)
	{
		static bool LoadTitleData = false;
		if (!LoadTitleData)
		{
			static CParser parser;

			if (!parser.LoadFile("gameTitle.txt"))
			{
				printf("게임 오버 파일 불러오기 실패");
				return;
			}

			bool bSuccess = false;
			do {
				if (!parser.GetValue("xpos", &curTitle.xpos))
				{
					break;
				}
				if (!parser.GetValue("ypos", &curTitle.ypos))
				{
					break;
				}
				if (!parser.GetString("GameTitlemessage", curTitle.message))
				{
					break;
				}

				bSuccess = true;
			} while (0);

			if (!bSuccess)
			{
				printf("게임 타이틀 메시지 로딩 실패\n");
				return;
			}
			LoadTitleData = true;
		}

		CurScene = Title;
	}
	else if (nextScene == Game)
	{
		static bool LoadGameData = false;

		if (!LoadGameData)
		{
			if (!LoadPattern(PatternType))
			{
				printf("페턴 타입 불러오기 실패\n");
				return;
			}

			if (!LoadEnemys(EnemyType))
			{
				printf("적 타입 불러오기 실패\n");
				return;
			}

			LoadStageInfo();

			LoadGameData = true;
		}
		LoadPlayer(&curPlayer);
		LoadStage(&StagePool[CurStageIndex]);
		CurScene = Game;
	}
	else if (nextScene == GameOver)
	{
		static bool LoadOverData = false;
		if (!LoadOverData)
		{
			CParser parser;

			if (!parser.LoadFile("gameOver.txt"))
			{
				printf("게임 오버 파일 불러오기 실패");
				return;
			}

			bool bSuccess = false;
			do {
				if (!parser.GetValue("xpos", &curGameOver.xpos))
				{
					break;
				}
				if (!parser.GetValue("ypos", &curGameOver.ypos))
				{
					break;
				}
				if (!parser.GetString("GameOvermessage", curGameOver.message))
				{
					break;
				}

				bSuccess = true;
			} while (0);

			if (!bSuccess)
			{
				printf("게임 오버 메시지 로딩 실패\n");
				return;
			}
			LoadOverData = true;
		}
		CurScene = GameOver;
	}
}

void GameReset()
{
	for (int i = 0; i < MAXBULLETNUM; i++)
	{
		if (BP[i].Active)
		{
			BP[i].Active = false;
		}
	}
	for (int i = 0; i < MAXENEMYNUM; i++)
	{
		if (EP[i].Active)
		{
			EP[i].Active = false;
		}
	}
	curPlayer.Active = false;
}