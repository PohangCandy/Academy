#include <stdio.h>

typedef unsigned short      WORD;

int main()
{
	WORD	_SectorY = -1;

	if (_SectorY == 0xffff)
	{
		printf("정상 결과 출력\n");
	}
	else
	{
		printf("비정상 결과 출력\n");
	}
	return 0;
}