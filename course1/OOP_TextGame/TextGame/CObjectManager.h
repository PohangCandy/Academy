#pragma once
#include <iostream>
#include "TemplateLinkedList.h"
#include "CBaseObject.h"

using namespace std;
//---------------------------------------
// 오브젝트들을 관리할 클래스 전역 또는 싱글톤
// 오브젝트들의 행동을 명령하고 생성, 삭제 관리
//---------------------------------------
class CObjectManager{
public:
	//--------------------------------------------
	// 인자 타입에 따른 객체 생성 new 후 List.push
	//--------------------------------------------
	void CreateObject(CBaseObject* bp);
	//--------------------------------------
	// 특정 객체를 찾아서 삭제 시킨 후 List.erase
	//--------------------------------------
	void DestroyObject(CBaseObject* target);

	void Update();		// ObjectList 를 순회하며 Update 호출
	void Render();		// ObjectList 를 순회하며 Render 호출

	void DeletePendingObjects();

	static CObjectManager* GetInstance()
	{
		if (instance == nullptr)
		{
			instance = new CObjectManager;
			atexit(Destroy);
		}
		return instance;
	}

	static void Destroy()
	{
		delete instance;
		instance = nullptr;
	}

	void Clear();

private:
	enum EObjectType {
		PLAYER,
		ENEMY,
		BULLET
	};

	CList<CBaseObject*>   _ObjectList;  //여러분이 만든 이터레이터 패턴의 List 를 사용

	static CObjectManager* instance;

	CObjectManager() {};
	~CObjectManager() {};
};
