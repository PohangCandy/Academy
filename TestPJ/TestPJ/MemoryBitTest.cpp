//---------------------------------------------------------------------------------------------
// 프로젝트명: 메모리 주소에서 사용되지 않는 비트 사용하기
// 
// 목적: 
// 1. 사용되지 않는 메모리 주소에 마스킹을 씌워도 되는지.
// 2.해당 마스킹이 씌워진 주소가 마스킹을 씌우지 않는 주소와 사용되지 않는 메모리 주소를 제외하고
// 너머지는 같게 동작하는지
// 
// 방법 : 
// 포인터 주소의 17비트에 마스킹을 통해 1씩 더하는 효과를 준다.
// 이때 해당 객체의 맴버가 동일한지 확인한다.
// 
// 결론 : 
//
//---------------------------------------------------------------------------------------------

#include "stdafx.h"

#define USERBIT 0x007fffffffffff;

class Ctest {
public:
	int data = 100;
};

int cnt = 0;

int main() {
	//Ctest* pct = new Ctest;
	//long long upper = (long long)(pct);
	//upper >>= (64 - 17));
	//upper = (upper + 1) & 0x1FFFF;
	//
	//long long lower = (long long)pct;
	//pct = (Ctest*)((upper << (64 - 17) | lower));

	while (1)
	{
		long long upper_17bit = InterlockedIncrement((long*)&cnt);

		//printf("push 진행중\n");
		Ctest* newTop = new Ctest;
		Ctest* ptop;

		//2.newTop의 하위 비트 저장
		long long lower_47bit = ((long long)newTop & 0x007fffffffffff);
		Ctest* UserBit = (Ctest*)lower_47bit;

		//3.CAS하기 전에 newTop에 들어갈 주소에 cnt를 나타내는 17비트 세팅
		newTop = (Ctest*)((upper_17bit << (64 - 17)) | lower_47bit);
	}
}