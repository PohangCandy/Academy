#include <stdio.h>

typedef unsigned short      WORD;

int main()
{

	WORD	_SectorY = -1;

	int useSize = 0x0000000d;
	unsigned short len = 0x000e;

	if (useSize < len){
	
		printf("정상 결과 출력\n");
	}
	else {
		printf("비정상 결과 출력\n");
	}

	int WordToInt = (int)_SectorY;
	int minusOne = -1;

	if (_SectorY == -1) {
		printf("정상 결과 출력\n");
	}
	else {
		printf("비정상 결과 출력\n");
	}
	return 0;
}