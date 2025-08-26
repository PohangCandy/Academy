#pragma once
#include "CSceneBase.h"
#include "CObjectManager.h"
#include "Stage.h"
#include "Player.h"
#include "Enemy.h"
#include "Bullet.h"
#include "FixedUpdate.h"
#include "CScreenBuffer.h"
#include "CSceneManager.h"

class CSceneGame : public CSceneBase
{
public:
	CSceneGame():CSceneBase(Game)
	{
		//게임씬에 맞는 기본 객체 생성 및 초기화 등등
	}

	~CSceneGame()
	{
		//동적할당된 맴버들 죄다 정리
	}

	void init();

	void Update();

	void StageReset();
private:
	bool bIsLoaded = false;
};


//--------------------------------------------------------------------
//총알 메모리풀
//--------------------------------------------------------------------
extern CBullet BP[MAXBULLETNUM];
//--------------------------------------------------------------------
//스테이지 적 파일 데이터
//--------------------------------------------------------------------
extern CEnemy EP[MAXENEMYNUM];

extern CObjectManager* CurObjManager;
extern CPlayer curPlayer;
extern CfixedUpdate fu;
extern CScreenBuffer* CurScreen;

//--------------------------------------------------------------------
//게임 씬의 현재 스테이지 인덱스
//--------------------------------------------------------------------
extern int CurStageIndex;