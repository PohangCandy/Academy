#pragma once
#include <wtypes.h>

#define DEFAULT_MOVE_SPEED_X 3
#define DEFAULT_MOVE_SPEED_Y 2
#define dfERROR_RANGE		30

#define dfATTACK1_RANGE_X		80
#define dfATTACK2_RANGE_X		90
#define dfATTACK3_RANGE_X		100
#define dfATTACK1_RANGE_Y		10
#define dfATTACK2_RANGE_Y		10
#define dfATTACK3_RANGE_Y		20

#define dfRANGE_MOVE_TOP	50
#define dfRANGE_MOVE_LEFT	10
#define dfRANGE_MOVE_RIGHT	630
#define dfRANGE_MOVE_BOTTOM	470

#define dfATTACK1_COOLDOWN 260
#define dfATTACK2_COOLDOWN 340
#define dfATTACK3_COOLDOWN 500
#define dfATTACK_NETWORK_DELAY 50
enum AttackType
{
	ATTACK1, ATTACK2, ATTACK3
};

class Player
{
public:
	Player(SOCKET socket, ULONG ip, short port, UINT id, BYTE dir, USHORT x, USHORT y, BYTE hp)
		: _socket(socket), _ip(ip), _port(port),
		_id(id), _dir(dir), _atkdir(dir), _x(x), _y(y), _hp(hp) {
	}

public:
	SOCKET _socket;
	ULONG _ip;
	USHORT _port;

	UINT _id;
	BYTE _dir;
	bool _atkdir; // false: left, true: right
	USHORT _x, _y;
	SHORT _hp;
};

