#pragma once
#include <Windows.h>
#include "Bullet.h"
#include "TextParser.h"
#include "CBaseObject.h"

#define MAXPlAYERNUM 1
#define MAXPlAYERTYPE 1


//--------------------------------------------------------------------
// 플레이어의 정보
// 
// 플레이어의 모양
// 플레이어의 시작 위치
// 플레이어의 hp
// 플레이어의 활성화
//--------------------------------------------------------------------
class CPlayer : public CBaseObject {
public:
	CPlayer() : CBaseObject(_ObjectType, _X, _Y) {}
	~CPlayer() {}

	virtual bool		Update(void) override;
	virtual void		Render(void) override;
	virtual void		OnCollision(CBaseObject* other) override;

	//--------------------------------------------------------------------
	// 키 입력에 따라 플레이어의 위치 좌표 이동
	//--------------------------------------------------------------------
	void MovePlayer(CPlayer* p, CBullet* bp);

	//--------------------------------------------------------------------
	// 플레이어와 총알 충돌 체크
	//--------------------------------------------------------------------
	void CheckPlayerHit(CBullet* bp, CPlayer* p);

	//--------------------------------------------------------------------
	// 플레이어 총알 발사
	//--------------------------------------------------------------------
	void PlayerFire(CPlayer* p, CBullet* bp);

	//--------------------------------------------------------------------
	// 플레이어 파일 데이터 로드
	//--------------------------------------------------------------------
	bool LoadPlayer(CPlayer* p);

	//--------------------------------------------------------------------
	// 플레이어 사망 체크
	//--------------------------------------------------------------------
	bool CkeckGameOver(CPlayer* p);

protected:
	int _ObjectType = 0;
	int _X = 0;
	int _Y = 0;

	char _shape = '@';
	int _hp = 0;
	bool _Active = 0;
};