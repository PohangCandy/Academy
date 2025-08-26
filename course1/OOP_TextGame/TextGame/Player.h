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
	CPlayer() : CBaseObject(PLAYER, 40, 23, 1) {}
	~CPlayer() {}


	virtual bool		Update(void) override;
	virtual void		Render(void) override;
	virtual void		OnCollision(CBaseObject* other) override;

	//--------------------------------------------------------------------
	// 키 입력에 따라 플레이어의 위치 좌표 이동
	//--------------------------------------------------------------------
	void MovePlayer();

	//--------------------------------------------------------------------
	// 플레이어와 총알 충돌 체크
	//--------------------------------------------------------------------
	void CheckPlayerHit(CBullet* bp, CPlayer* p);

	//--------------------------------------------------------------------
	// 플레이어 총알 발사
	//--------------------------------------------------------------------
	void PlayerFire();

	//--------------------------------------------------------------------
	// 플레이어 파일 데이터 로드
	//--------------------------------------------------------------------
	static bool LoadPlayer(CPlayer* p);

	//--------------------------------------------------------------------
	// 플레이어 사망 체크
	//--------------------------------------------------------------------
	static bool CheckGameOver(CPlayer* p);

	void Deactivate(void) { _Active = false; }
	bool IsAvailable(void) { return _Active; };

protected:
	char _shape = 'P';
	int _hp = 5;
};