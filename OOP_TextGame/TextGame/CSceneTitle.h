#pragma once
#include "CSceneBase.h"
#include "FixedUpdate.h"
#include "CScreenBuffer.h"
#include "CSceneManager.h"
#include "TextParser.h"


class CSceneTitle : public CSceneBase
{
public:
	void init();
	void Update();
	CSceneTitle():CSceneBase(Title){}

private:
	bool bIsLoaded = false;

	char message[256] = "0";
	int xpos = 0;
	int ypos = 0;
};

extern CfixedUpdate fu;
extern CScreenBuffer* CurScreen;
