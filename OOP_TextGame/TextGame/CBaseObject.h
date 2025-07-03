#pragma once


class CBaseObject
{
public:

	CBaseObject(int ObjectType, int X, int Y, bool Active);
	virtual ~CBaseObject();

	virtual bool		Update(void) = 0;
	virtual void		Render(void) = 0;
	virtual void		OnCollision(CBaseObject* other) = 0;

	virtual int	 GetObjectType();

	int GetPos_X() { return _X; }
	int GetPos_Y() { return _Y; }

	bool IsPendingDelete() { return !_Active; };

protected:

	bool _Active;
	int 	_X;
	int 	_Y;

	enum EObjectType {
		PLAYER,
		ENEMY,
		BULLET
	};
	int 	_EObjectType;
};