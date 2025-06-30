#pragma once


class CBaseObject
{
public:

	CBaseObject(int ObjectType, int X, int Y);
	virtual ~CBaseObject();

	virtual bool		Update(void) = 0;
	virtual void		Render(void) = 0;
	virtual void		OnCollision(CBaseObject* other) = 0;

	int	 GetObjectType();

protected:

	int 	_iX;
	int 	_iY;
	int 	_ObjectType;
};