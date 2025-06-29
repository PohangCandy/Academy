#pragma once
//루프 순회마다 프레임이 보정되는 로직

//문제점:  현재 초를 기준으로 skip유무를 결정하므로
// 2초 이상 정지되지 않으면 skip하지 않는다.
//프레임이 느려지고 복구 될 때 프레임이 튀지않고, 다시 정상 복구한다. 
//한 루프를 돌때마다 시간을 체크해서 그게 2초 이상이면 밀린 프레임을 더해주는 로직이었다.
//루프는 20ms 마다 돌고있으므로 2초이상 루프가 걸렸냐? 고 물어보는 의미없는 로직이었다.
//프레임이 초당 나올수있는 장면 수인데, 프레임이 밀린다는 의미가 모호하다.
//ex) 1초에 50번 나와야하는데 30번 밖에 안나왔으니 다음에 20번 더 더해라 -> 어떻게? 랜더를 포기해서
//ex) 20ms동안 1번 랜더해야하는데 랜더하는데 두 배인 40ms 이상 걸렸으니 초과된 시간만큼 다음 프레임의 랜더 포기해라
//전자의 경우 1초 단위가 기준이 되므로, 1초동안 랜더한 횟수를 저장했다가 다음 초에 초과분을 계산해야 한다.
//후자의 경우 시작 시간으로부터 20ms 단위가 기준이 되어 매 루프마다 랜더를 할지말지 결정해야한다.

#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <time.h>
#include <windows.h>
#include "Console.h"

#pragma comment(lib, "winmm.lib") 

class CfixedUpdate
{
	int frameByMs = 100;
	int OneSecond = 1000;

public:
	char Framemessage[20];

	unsigned int FrameBeginTime;

	bool Skip = false;

	void Logic();

	void Render();

	void AddTimeAndCount(int* time, int* cnt);

	void Frame();
};