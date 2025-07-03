#pragma once
#include "CSceneBase.h"
#include "FixedUpdate.h"
#include "CScreenBuffer.h"
#include "TextParser.h"
#include "CSceneManager.h"

class CSceneOver : public CSceneBase
{
public:
	CSceneOver():CSceneBase(Over){	}
	void init();
	void Update();
private:
	bool bIsLoaded = false;

	char message[256] = "0";
	int xpos = 0;
	int ypos = 0;
};

extern CfixedUpdate fu;
extern CScreenBuffer* CurScreen;