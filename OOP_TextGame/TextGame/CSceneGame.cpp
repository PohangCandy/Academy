#include "CSceneGame.h"

void CSceneGame::init()
{
	if (!bIsLoaded)
	{
		static bool LoadGameData = false;

		if (!LoadGameData)
		{
			if (!LoadPattern(PatternType))
			{
				printf("페턴 타입 불러오기 실패\n");
				return;
			}

			if (!CEnemy::LoadEnemys(EnemyType))
			{
				printf("적 타입 불러오기 실패\n");
				return;
			}

			StagePool = (tag_Stage*)malloc(sizeof(tag_Stage) * MAXSTAGENUM);
			LoadStageInfo();

			if (!CBullet::loadBullet(BulletType))
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
		CPlayer::LoadPlayer(&curPlayer);
		if (StagePool != NULL) {
			LoadStage(&StagePool[CurStageIndex]);
		}

		CurObjManager->CreateObject(&curPlayer);
		for (int i = 0; i < MAXENEMYNUM; i++)
		{
			if (EP[i].IsAvailable())
			{
				CurObjManager->CreateObject(&EP[i]);
			}
		}
		bIsLoaded = true;
	}
}

void CSceneGame::Update()
{
	// 1. 키보드 입력부
	CurObjManager->Update();
	if (Stage_End())
	{
		if (CurStageIndex + 1 < CurStageNum)
		{
			StageReset();
			CurStageIndex++;
			CSceneManager::GetInstance()->LoadScene(Game);
			return;
		}
		else
		{
			StageReset();
			CurStageIndex = 0;
			CSceneManager::GetInstance()->LoadScene(Over);
			return;
		}
	}
	//CheckPlayerHit(BP, &curPlayer);
	if (CPlayer::CheckGameOver(&curPlayer))
	{
		StageReset();
		CurStageIndex = 0;
		CSceneManager::GetInstance()->LoadScene(Over);
		return;
	}

	if (!fu.Skip)
	{
		// 3. 랜더부
			// 스크린 버퍼를 지움
		CurScreen->Buffer_Clear();
		// 스크린 버퍼에 객체들 출력
		CurObjManager->Render();
		//// 스크린 버퍼를 화면으로 출력
		fu.Render();
		for (int i = 0; i < (signed int)strlen(fu.Framemessage); i++)
		{
			CurScreen->Sprite_Draw(30 + i, 0, fu.Framemessage[i]);
		}

		CurScreen->Buffer_Flip();
	}
}

void CSceneGame::StageReset()
{
	CurObjManager->Clear();

	for (int i = 0; i < MAXBULLETNUM; i++)
	{
		if (BP[i].IsAvailable())
		{
			BP[i].Deactivate();
		}
	}
	for (int i = 0; i < MAXENEMYNUM; i++)
	{
		if (EP[i].IsAvailable())
		{
			EP[i].Deactivate();
		}
	}
	curPlayer.Deactivate();
}
