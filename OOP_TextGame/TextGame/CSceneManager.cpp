#include "CSceneManager.h"
#include "CSceneGame.h"
#include "CSceneOver.h"
#include "CSceneTitle.h"

CSceneManager* CSceneManager::instance = nullptr;

void CSceneManager::run()
{
	if (_nextScene)
	{
		if (_pScene)
		{
			delete _pScene;
		}

		_pScene = _nextScene;
		_nextScene = nullptr;

		_pScene->init();
	}

	if (_pScene)
	{
		_pScene->Update(); 
	}

}

//-------------------------------------------
//씬 전환 정보 셋팅.
//-------------------------------------------
void CSceneManager::LoadScene(int newScene)
{
	if (_nextScene)
	{
		delete _nextScene;
		_nextScene = nullptr;
	}

	switch (newScene)
	{
	case Title:
		_nextScene = new CSceneTitle;
		break;
	case Game:
		_nextScene = new CSceneGame;
		break;
	case Over:
		_nextScene = new CSceneOver;
		break;
	default:
		break;
	}
}
