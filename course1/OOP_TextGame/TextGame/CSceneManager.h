#pragma once
#include "CSceneBase.h"

class CSceneManager
{
public:
	void run();

	//ÀüÈ¯ ¾À
	void LoadScene(int newScene);

	static CSceneManager* GetInstance()
	{
		if (CSceneManager::instance == nullptr)
		{
			instance = new CSceneManager;
		}
		return instance;
	}
private:
	CSceneBase* _pScene = nullptr;
	CSceneBase* _nextScene = nullptr;
	enum _ESceneType
	{
		Load,
		Title,
		Game,
		Over
	};
	CSceneManager(){}
	static CSceneManager* instance;
};
