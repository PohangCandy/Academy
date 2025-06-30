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
#include "FixedUpdate.h"


#pragma comment(lib, "winmm.lib") 

//0.로딩씬에서 패턴 못 읽어오고 있음.
//1.에서 소멸자에서 filedata를 동적해제하는 바람에 나머지 패턴을 읽어오지 못함.
//나머지 데이터 읽기전에 소멸자가 호출됨.
//객체의 소멸자가 호출된 이유는 객체를 복사해서 넘겨주고 있기 때문임.
//-> 복사가 아닌 포인터를 넘겨주면 생성자, 소멸자가 호출되지 않음.
//지금보니까 구조체나 객체를 그냥 인자로넘기는 경우가 많아서 포인터로 바꿔줘야 할 듯

//1. 텍스트 파서 동적 메모리 해제하기
//-> 소멸자에서 동적 해제하도록 해결

//2. Static 지역 변수로 선언된 텍스트 파서 리팩토링
//크기가 큰 객체나 배열 함수 안에 선언할 때 여러번 초기화되지 않도록 static으로 선언했는데
//어차피 계속 초기화 되는 작업이라 지역으로 선언해도 될 듯.

//3. 전역 변수를 인자로 받는 함수 리팩토링
//-> 객체 지향이되면 전역 인자로 안받으니까 이렇게 처리해야 하지 않을까?

//4. 적이 움직여서 안죽는 총알 안맞는 현상 발생
// 적의 이전 위치도 로직 고려해주기
// 
// 해결 방법
// 1. 오브젝트가 움직일때마다 충돌체크.
// 2. 오브젝트가 이전 위치 정보에 대한 데이터를 가짐. 
// 충돌 체크 고려시 오브젝트의 이전 위치 정보 고려
// 충돌 체크를 한 구간에서 처리하는 로직을 사용함. 

//5. OnCollision 충돌을 위해 객체 타입을 int형을 정의
//0 = player
//1 = enemy
//2 = bullet

//6.InitBullet()
//bullet이 생성 될 때, 설정을 초기화 해주는 함수 InitBullet() 완성
//이런 식으로 현재 설정하거나 불러와야 하는 값들을 get,set이 아닌 내부적으로 완성해줘야함.
//근데 하면서 느끼는 건데 가독성 개 떨어짐.
//적이 총알의 맴버를 건들이면 안되는건 맞지만
//내부적으로 처리해주려고 함수에 인자로 값만 보내는 것만 보고 무슨 코드인지 판단하기 개힘듬.
//tb->InitBullet(true, ep[i]._X, ep[i]._Y, 1);

//Todo. FindBullet()
//FindBullet을 어떻게 구현해줄까?
//각 객체들의 메모리 풀과 CBaseObjectList가 어떤 형태로 공존해야하는가... 


//--------------------------------------------------------------------
char szScreenBuffer[dfSCREEN_HEIGHT][dfSCREEN_WIDTH];


////--------------------------------------------------------------------
////플레이어 타입
////--------------------------------------------------------------------
//tag_Player PlayerType[MAXPlAYERTYPE];

////--------------------------------------------------------------------
////플레이어 메모리풀
////--------------------------------------------------------------------
CPlayer curPlayer;

//--------------------------------------------------------------------
//적 타입
//--------------------------------------------------------------------
CEnemy EnemyType[MAXENEMYTYPE];
//--------------------------------------------------------------------
//적 메모리풀
//--------------------------------------------------------------------
CEnemy EP[MAXENEMYNUM];


//--------------------------------------------------------------------
//총알 타입
//--------------------------------------------------------------------
CBullet BulletType[MAXBULLETTYPE];
//--------------------------------------------------------------------
//총알 메모리풀
//--------------------------------------------------------------------
CBullet BP[MAXBULLETNUM];

//--------------------------------------------------------------------
//패턴 타입
//--------------------------------------------------------------------
tag_Pattern PatternType[MAXPATTERNTYPENUM];

//--------------------------------------------------------------------
//스테이지 메모리풀
//--------------------------------------------------------------------
//tag_Stage StagePool[MAXSTAGENUM];
tag_Stage* StagePool;

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
//게임에서 읽어들인 모든 StageNum;
//--------------------------------------------------------------------
int CurStageNum;

//--------------------------------------------------------------------
//프레임 드랍을 해결해줄 FixedUpdate
//--------------------------------------------------------------------
CfixedUpdate fu;

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
void StageReset();

//--------------------------------------------------------------------
// 게임 데이터 동적 할당 해제
//--------------------------------------------------------------------
void EndGame();


int main(void)
{
	cs_Initial();
	CurScene = Load;
	nextScene = Title;
	CurStageIndex = 0;

	//-------------------------------------------------------------------
	// 게임의 메인 루프
	// 이 루프가  1번 돌면 1프레임 이다.
	// 1프레임 당 100ms, 10FPS로 맞춘다.
	//--------------------------------------------------------------------
	timeBeginPeriod(1);
	while (1)
	{
		/*unsigned int st = timeGetTime();*/
		fu.Logic();
		fu.Frame();
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
		//unsigned int et = timeGetTime();
		//// 프레임 맞추기용 대기 Sleep(X)
		//if (et - st < 100)
		//{
		//	Sleep(100 - (et - st));
		//}
	}
	timeEndPeriod(1);
	EndGame();
	return 0;
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
	CheckDamagedEnemy(BP, EP);
	if (Stage_End())
	{
		if (CurStageIndex + 1 < CurStageNum)
		{
			StageReset();
			CurStageIndex++;
			CurScene = Load;
			nextScene = Game;
			return;
		}
		else
		{
			StageReset();
			CurStageIndex = 0;
			CurScene = Load;
			nextScene = GameOver;
			return;
		}
	}
	CheckPlayerHit(BP, &curPlayer);
	if (CkeckGameOver(&curPlayer))
	{
		StageReset();
		CurStageIndex = 0;
		CurScene = Load;
		nextScene = GameOver;
		return;
	}

	if (!fu.Skip)
	{
		// 3. 랜더부
			// 스크린 버퍼를 지움
		Buffer_Clear();
		// 스크린 버퍼에 객체들 출력
		//DrawBullet
		for (int i = 0; i < MAXBULLETNUM; i++)
		{
			if (BP[i]._Active)
			{
				Sprite_Draw(BP[i]._X, BP[i]._Y, BP[i]._shape);
			}
		}

		//DrawEnemy
		for (int i = 0; i < MAXENEMYNUM; i++)
		{
			if (EP[i]._Active)
			{
				Sprite_Draw(EP[i]._X, EP[i]._Y, EP[i]._shape);
			}
		}

		//DrawPlayer
		if (curPlayer._Active)
		{
			Sprite_Draw(curPlayer._X, curPlayer._Y, curPlayer._shape);
		}
		// 스크린 버퍼를 화면으로 출력
		fu.Render();
		for (int i = 0; i < (signed int)strlen(fu.Framemessage); i++)
		{
			Sprite_Draw(30 + i, 0, fu.Framemessage[i]);
		}
		Buffer_Flip();
	}
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

	if (!fu.Skip)
	{
		//랜더
		Buffer_Clear();
		for (int i = 0; i < (signed int)strlen(curTitle.message); i++)
		{
			Sprite_Draw(curTitle.xpos + i, curTitle.ypos, curTitle.message[i]);
		}
		fu.Render();
		for (int i = 0; i < (signed int)strlen(fu.Framemessage); i++)
		{
			Sprite_Draw(30 + i, 0, fu.Framemessage[i]);
		}
		Buffer_Flip();
	}
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
	if (!fu.Skip)
	{
		Buffer_Clear();
		for (int i = 0; i < (signed int)strlen(curGameOver.message); i++)
		{
			Sprite_Draw(curGameOver.xpos + i, curGameOver.ypos, curGameOver.message[i]);
		}
		fu.Render();
		for (int i = 0; i < (signed int)strlen(fu.Framemessage); i++)
		{
			Sprite_Draw(30 + i, 0, fu.Framemessage[i]);
		}
		Buffer_Flip();
	}
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

			StagePool = (tag_Stage*)malloc(sizeof(tag_Stage) * MAXSTAGENUM);
			LoadStageInfo();

			if (!loadBullet(BulletType))
			{
				printf("총알 타입 불러오기 실패\n");
				return;
			}

			for (int i = 0; i < MAXBULLETNUM; i++)
			{
				BP[i] = BulletType[0];
			}

			LoadGameData = true;
		}
		LoadPlayer(&curPlayer);
		if (StagePool != NULL) {
			LoadStage(&StagePool[CurStageIndex]);
		}

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

	if (!fu.Skip)
	{
		fu.Render();
		for (int i = 0; i < (signed int)strlen(fu.Framemessage); i++)
		{
			Sprite_Draw(30 + i, 0, fu.Framemessage[i]);
		}
	}
}

void StageReset()
{
	for (int i = 0; i < MAXBULLETNUM; i++)
	{
		if (BP[i]._Active)
		{
			BP[i]._Active = false;
		}
	}
	for (int i = 0; i < MAXENEMYNUM; i++)
	{
		if (EP[i]._Active)
		{
			EP[i]._Active = false;
		}
	}
	curPlayer._Active = false;
}

//--------------------------------------------------------------------
// 게임 데이터 동적 할당 해제
//--------------------------------------------------------------------
void EndGame()
{
	free(StagePool);
}
