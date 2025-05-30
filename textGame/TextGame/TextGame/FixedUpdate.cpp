#include "FixedUpdate.h"

void CfixedUpdate::Logic()
{
	FrameBeginTime = timeGetTime();
	//Logic
}

void CfixedUpdate::Render()
{
	//Render
	unsigned int curTime = timeGetTime();

	//빠르다.
	//쉬어가자.
	if (curTime - FrameBeginTime < frameByMs)
	{
		//빠른 만큼 쉬어가자.
		Sleep(frameByMs - (curTime - FrameBeginTime));
	}
}

void CfixedUpdate::AddTimeAndCount(int* time, int* cnt)
{
	printf("%d\n", *cnt);
	*cnt = 0;
	*time += OneSecond;
}

void CfixedUpdate::Frame()
{
	static int cnt = 0;
	//static int Goalcnt = 0;

	//프레임 출력하기 위한 1초 단위 시간
	static int ThreadRunTimeSec = timeGetTime();

	//로직 시간 계산하기 위해
	//제일 처음 로직 시작 시간을 저장
	static int LoopRunTimemileSec = FrameBeginTime;

	int curTime = timeGetTime();
	cnt++;

	//프레임이 밀려서 현재 시간과 정상 작동 시간 차가 많이 발생하면
	if (curTime - LoopRunTimemileSec >= frameByMs * 2)
	{
		//랜더와 sleep을 없애고 프레임 따라잡을때까지 로직을 돌린다.
		Skip = true;
		//
		LoopRunTimemileSec += frameByMs;
	}
	//로직이 끝나야 하는 정상시간
	else if (curTime - LoopRunTimemileSec >= frameByMs)
	{
		Skip = false;
		LoopRunTimemileSec += frameByMs;
	}

	//스킵을 하는 경우는 프레임이 완전히 1프레임 밀렸을 때 임.
	//1프레임이 밀렸다는건 무슨 의미일까
	//20ms 동안 1로직이 돌아야하는데 32ms 동안 돌았다. -> 12ms를 초과했다.
	//다음 프레임은 원래 20ms~40ms까지 돌아야하는데 32ms부터 시작한다.
	//만약 이 범위를 초과해서 1로직이 40ms까지 돌아버린 경우
	//2프레임은 3프레임 위치에 들어가게되므로 이걸 프레임이 밀렸다고 가정한다.
	//그럼 프레임이 밀렸는지 체크하는건 
	//로직이 수행 될 때마다 20ms씩 더해지는 변수를 두고
	//현재 로직이 해당 범위내에서 수행되는지를 체크해야하지 않을까?


	//1초마다 프레임 출력
	if (curTime - ThreadRunTimeSec >= OneSecond)
	{
		sprintf_s(Framemessage,sizeof(Framemessage), "평균 프레임 : %d\n", cnt);
		cnt = 0;
		ThreadRunTimeSec += OneSecond;
	}
}