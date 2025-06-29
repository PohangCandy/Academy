#pragma once
#include <Windows.h>
#include "Bullet.h"
#include "TextParser.h"
#include "CBaseObject.h"

#define MAXPlAYERNUM 1
#define MAXPlAYERTYPE 1

class CPlayer : public CBaseObject {
public:
	CPlayer() :CBaseObject(ObjectType, m_X, m_Y) {}
	~CPlayer() {}

	virtual bool		Update(void) override;
	virtual void		Render(void) override;
	virtual void		OnCollision(CBaseObject* other) override;

	int ObjectType = 0;
	char m_shape = '@';
	int m_X = 0;
	int m_Y = 0;
	int m_hp = 0;
	bool m_Active = 0;
};

//--------------------------------------------------------------------
// 플레이어의 정보
// 
// 플레이어의 모양
// 플레이어의 시작 위치
// 플레이어의 hp
// 플레이어의 활성화
//--------------------------------------------------------------------
struct tag_Player
{
	char shape = '@';
	int x = 0;
	int y = 0;
	int hp = 0;
	bool Active = 0;
};

//--------------------------------------------------------------------
// 키 입력에 따라 플레이어의 위치 좌표 이동
//--------------------------------------------------------------------
void MovePlayer(tag_Player* p, tag_Bullet* bp);

//--------------------------------------------------------------------
// 플레이어와 총알 충돌 체크
//--------------------------------------------------------------------
void CheckPlayerHit(tag_Bullet* bp, tag_Player* p);

//--------------------------------------------------------------------
// 플레이어 총알 발사
//--------------------------------------------------------------------
void PlayerFire(tag_Player* p, tag_Bullet* bp);

//--------------------------------------------------------------------
// 플레이어 파일 데이터 로드
//--------------------------------------------------------------------
bool LoadPlayer(tag_Player* p);

//--------------------------------------------------------------------
// 플레이어 사망 체크
//--------------------------------------------------------------------
bool CkeckGameOver(tag_Player* p);