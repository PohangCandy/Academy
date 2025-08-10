#pragma once
#include <Windows.h>

#pragma pack(push, 1)

class PacketBase
{
public:
	PacketBase() : _size(0), _type(0) {}
	PacketBase(BYTE size, BYTE type) : _size(size), _type(type) {}

	BYTE _code = 0x89;
	BYTE _size; // size
	BYTE _type; // type
};

#define	dfPACKET_SC_CREATE_MY_CHARACTER			0
class PacketCreateMyCharacter : public PacketBase
{
public:
	PacketCreateMyCharacter(UINT id, BYTE direction, USHORT x, USHORT y, BYTE hp)
		: PacketBase(sizeof(PacketCreateMyCharacter) - sizeof(PacketBase),
			dfPACKET_SC_CREATE_MY_CHARACTER),
		_id(id), _direction(direction), _x(x), _y(y), _hp(hp) {
	}

	UINT _id;
	BYTE _direction;
	USHORT _x;
	USHORT _y;
	BYTE _hp;
};

#define	dfPACKET_SC_CREATE_OTHER_CHARACTER		1
class PacketCreateOtherCharacter : public PacketBase
{
public:
	PacketCreateOtherCharacter(INT id, BYTE direction, USHORT x, USHORT y, BYTE hp)
		: PacketBase(sizeof(PacketCreateOtherCharacter) - sizeof(PacketBase),
			dfPACKET_SC_CREATE_OTHER_CHARACTER),
		_id(id), _direction(direction), _x(x), _y(y), _hp(hp) {
	}

	UINT _id;
	BYTE _direction;
	USHORT _x;
	USHORT _y;
	BYTE _hp;
};

#define	dfPACKET_SC_DELETE_CHARACTER			2
class PacketDeleteCharacter : public PacketBase
{
public:
	PacketDeleteCharacter(UINT id) : PacketBase(sizeof(PacketDeleteCharacter) - sizeof(PacketBase),
		dfPACKET_SC_DELETE_CHARACTER), _id(id)
	{
	}
	UINT _id;
};

#define	dfPACKET_CS_MOVE_START					10
#define dfPACKET_MOVE_DIR_LL					0
#define dfPACKET_MOVE_DIR_LU					1
#define dfPACKET_MOVE_DIR_UU					2
#define dfPACKET_MOVE_DIR_RU					3
#define dfPACKET_MOVE_DIR_RR					4
#define dfPACKET_MOVE_DIR_RD					5
#define dfPACKET_MOVE_DIR_DD					6
#define dfPACKET_MOVE_DIR_LD					7
class PacketMoveStartCtoS : public PacketBase
{
public:
	PacketMoveStartCtoS() : PacketBase(sizeof(PacketMoveStartCtoS) - sizeof(PacketBase),
		dfPACKET_CS_MOVE_START)
	{
	}

	BYTE _direction;
	USHORT _x;
	USHORT _y;
};

#define	dfPACKET_SC_MOVE_START					11
class PacketMoveStartStoC : public PacketBase
{
public:
	PacketMoveStartStoC(PacketMoveStartCtoS pktCtoS, UINT id)
		: PacketBase(sizeof(PacketMoveStartStoC) - sizeof(PacketBase), dfPACKET_SC_MOVE_START),
		_id(id), _direction(pktCtoS._direction), _x(pktCtoS._x), _y(pktCtoS._y)
	{
	}
	UINT _id;
	BYTE _direction;
	USHORT _x;
	USHORT _y;
};

#define	dfPACKET_CS_MOVE_STOP					12
class PacketMoveStopCtoS : public PacketBase
{
public:
	PacketMoveStopCtoS() : PacketBase(sizeof(PacketMoveStopCtoS) - sizeof(PacketBase),
		dfPACKET_CS_MOVE_STOP)
	{
	}

	BYTE _direction;
	USHORT _x;
	USHORT _y;
};

#define	dfPACKET_SC_MOVE_STOP					13
class PacketMoveStopStoC : public PacketBase
{
public:
	PacketMoveStopStoC(PacketMoveStopCtoS pktCtoS, UINT id)
		: PacketBase(sizeof(PacketMoveStopStoC) - sizeof(PacketBase), dfPACKET_SC_MOVE_STOP),
		_id(id), _direction(pktCtoS._direction), _x(pktCtoS._x), _y(pktCtoS._y)
	{
	}
	UINT _id;
	BYTE _direction;
	USHORT _x;
	USHORT _y;
};

#define	dfPACKET_CS_ATTACK1						20
class PacketAttack1CtoS : public PacketBase
{
public:
	PacketAttack1CtoS() : PacketBase(sizeof(PacketAttack1CtoS) - sizeof(PacketBase),
		dfPACKET_CS_ATTACK1)
	{
	}

	BYTE _direction;
	USHORT _x;
	USHORT _y;

};

#define	dfPACKET_SC_ATTACK1						21
class PacketAttack1StoC : public PacketBase
{
public:
	PacketAttack1StoC(PacketAttack1CtoS pktCtoS, UINT id)
		: PacketBase(sizeof(PacketAttack1StoC) - sizeof(PacketBase), dfPACKET_SC_ATTACK1),
		_id(id), _direction(pktCtoS._direction), _x(pktCtoS._x), _y(pktCtoS._y)
	{
	}
	UINT _id;
	BYTE _direction;
	USHORT _x;
	USHORT _y;
};

#define	dfPACKET_CS_ATTACK2						22
class PacketAttack2CtoS : public PacketBase
{
public:
	PacketAttack2CtoS() : PacketBase(sizeof(PacketAttack2CtoS) - sizeof(PacketBase),
		dfPACKET_CS_ATTACK2)
	{
	}

	BYTE _direction;
	USHORT _x;
	USHORT _y;

};

#define	dfPACKET_SC_ATTACK2						23
class PacketAttack2StoC : public PacketBase
{
public:
	PacketAttack2StoC(PacketAttack2CtoS pktCtoS, UINT id)
		: PacketBase(sizeof(PacketAttack2StoC) - sizeof(PacketBase), dfPACKET_SC_ATTACK2),
		_id(id), _direction(pktCtoS._direction), _x(pktCtoS._x), _y(pktCtoS._y)
	{
	}
	UINT _id;
	BYTE _direction;
	USHORT _x;
	USHORT _y;
};

#define	dfPACKET_CS_ATTACK3						24
class PacketAttack3CtoS : public PacketBase
{
public:
	PacketAttack3CtoS() : PacketBase(sizeof(PacketAttack3CtoS) - sizeof(PacketBase),
		dfPACKET_CS_ATTACK3)
	{
	}

	BYTE _direction;
	USHORT _x;
	USHORT _y;

};

#define	dfPACKET_SC_ATTACK3						25
class PacketAttack3StoC : public PacketBase
{
public:
	PacketAttack3StoC(PacketAttack3CtoS pktCtoS, UINT id)
		: PacketBase(sizeof(PacketAttack3StoC) - sizeof(PacketBase), dfPACKET_SC_ATTACK3),
		_id(id), _direction(pktCtoS._direction), _x(pktCtoS._x), _y(pktCtoS._y)
	{
	}
	UINT _id;
	BYTE _direction;
	USHORT _x;
	USHORT _y;
};

#define	dfPACKET_SC_DAMAGE						30
class PacketDamage : public PacketBase
{
public:
	PacketDamage(UINT attackID, UINT damageID, BYTE damageHP)
		:PacketBase(sizeof(PacketDamage) - sizeof(PacketBase), dfPACKET_SC_DAMAGE),
		_attackID(attackID), _damageID(damageID), _damageHP(damageHP) {
	}

	UINT _attackID;
	UINT _damageID;
	BYTE _damageHP;
};
#pragma pack(pop)