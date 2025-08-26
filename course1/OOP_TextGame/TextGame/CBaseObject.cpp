#include "CBaseObject.h"

CBaseObject::CBaseObject(int ObjectType, int X, int Y, bool Active)
{
	_X = X;
	_Y = Y;
	_EObjectType = ObjectType;
	_Active = Active;
}

CBaseObject::~CBaseObject()
{

}

int CBaseObject::GetObjectType()
{
	return _EObjectType;
}

