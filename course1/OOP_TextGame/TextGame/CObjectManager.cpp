#include "CObjectManager.h"
#include "Player.h"
#include "Bullet.h"
#include "Enemy.h"

CObjectManager* CObjectManager::instance = nullptr;

void CObjectManager::CreateObject(CBaseObject* bp)
{
	//스테이지 정보 유지하기 위해 미리 만들어진 객체를 push
	_ObjectList.push_back(bp);
}

void CObjectManager::DestroyObject(CBaseObject* target)
{
	for (CList<CBaseObject*>::iterator it = _ObjectList.begin(); it != _ObjectList.end(); ++it) {
		if (*it == target) {
			delete* it;
			_ObjectList.erase(it);
			break;
		}
	}
}

void CObjectManager::Update()
{
	CList<CBaseObject*>::iterator iter = _ObjectList.begin();

	for (; iter != _ObjectList.end(); ++iter)
	{
		CBaseObject*  pObject = *iter;
		pObject->Update();

		CList<CBaseObject*>::iterator target_iter = iter;

		for (++target_iter; target_iter != _ObjectList.end(); ++target_iter)
		{
			CBaseObject* pTargetObject = *target_iter;
			/*pObject 와 pTargetObject 의 충돌 판단
				충돌시 양쪽 객체에게 모두 이를 알려줌*/
			if (pTargetObject->GetPos_X() == pObject->GetPos_X() && pTargetObject->GetPos_Y() == pObject->GetPos_Y())
			{
				pObject->OnCollision(pTargetObject);
				pTargetObject->OnCollision(pObject);
			}
			//적이 안 맞는 문제 해결하기 위한 로직 추가
			if (pObject->GetObjectType() == ENEMY)
			{
				CEnemy* pc = (CEnemy*)pObject;
				if (pc->getexpos_X() == pTargetObject->GetPos_X() && pc->getexpos_Y() == pTargetObject->GetPos_Y())
				{
					pObject->OnCollision(pTargetObject);
					pTargetObject->OnCollision(pObject);
				}
			}
			//이후 충돌에 대한 로직은 각 객체가 알아서 하도록 함.
		}
	}

	//모든객체 Update 후 일괄 삭제
	DeletePendingObjects();

}

void CObjectManager::Render()
{

	CList<CBaseObject*>::iterator iter = _ObjectList.begin();

	for (; iter != _ObjectList.end(); ++iter)
	{
		CBaseObject* pObject = *iter;
		pObject->Render();
	}
}

void CObjectManager::DeletePendingObjects()
{
	for (CList<CBaseObject*>::iterator it = _ObjectList.begin(); it != _ObjectList.end(); ) {
		if ((*it)->IsPendingDelete()) {
			it = _ObjectList.erase(it);
		}
		else {
			++it;
		}
	}
}

void CObjectManager::Clear()
{
	_ObjectList.clear();
}
