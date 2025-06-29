#pragma once
#include "Console.h"
#include "TextParser.h"
#include "CBaseObject.h"

//--------------------------------------------------------------------
//총알 최대 수
//--------------------------------------------------------------------
#define MAXBULLETNUM 100
//--------------------------------------------------------------------
//총알 타입 수
//--------------------------------------------------------------------
#define MAXBULLETTYPE 1

class CBullet : public CBaseObject {
public:
	CBullet(int ObjectType, int X, int Y) :CBaseObject(ObjectType, X, Y) {};
	~CBullet() {};

	virtual bool		Update(void) override;
	virtual void		Render(void) override;
	virtual void		OnCollision(CBaseObject* other) override;

	bool bEnemy = false;
};


//--------------------------------------------------------------------
// 총알의 정보
// 
// 총알의 모양
// 총알의 위치
// 총알 종류
// 총알 방향
//--------------------------------------------------------------------
struct tag_Bullet
{
	char type[12] = "none";

	bool Active = false;

	char shape = 'O';
	char pshape = 'O';
	char eshape = 'O';
	int x = 0;
	int y = 0;
	bool bEnemy = false;
	int directionY = -1;
};

//--------------------------------------------------------------------
//총알 타입
//--------------------------------------------------------------------
extern tag_Bullet BulletType[MAXBULLETTYPE];

//--------------------------------------------------------------------
// 자동으로 총알 위치 좌표 이동
// 종류에 따라 위 or 아래로 움직임
// 플레이어는 스페이스로 총알 생성
// 적은 랜덤한 시간으로 총알 생성
//--------------------------------------------------------------------
void MoveBullet(tag_Bullet* bp);

//--------------------------------------------------------------------
// 총알 메모리 풀에서 사용가능한 총알 반환
//--------------------------------------------------------------------
tag_Bullet* FindBullet(tag_Bullet* bp);


//--------------------------------------------------------------------
// 총알 모양 파일에서 읽어서 메모리 풀 초기화
//--------------------------------------------------------------------
bool loadBullet(tag_Bullet b[]);