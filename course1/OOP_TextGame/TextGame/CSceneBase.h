#pragma once

class CSceneBase
{
public:
	CSceneBase(int sceneType) { _sceneType = sceneType; }

	virtual void init() = 0;
	virtual void Update() = 0;
	int Get_sceneType() 
	{
		return _sceneType;
	}

protected:
	//Load씬에서 씬 파일 데이터 받기 위해 타입 정보 구현
	int _sceneType;
	enum _ESceneType
	{
		Load,
		Title,
		Game,
		Over
	};
};