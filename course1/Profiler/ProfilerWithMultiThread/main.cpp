#include <iostream>
#include "MultiThreadProfiler.h"

void Test();

//굳이 static으로 숨길 필요없는데?
//PROFILE_SAMPLE arP[PROFILE_NUM];

int main()
{
	WCHAR c[] = L"func1";
	ProfileBegin(c);
	Test();
	ProfileEnd(c);

	ProfileBegin(c);
	Test();
	ProfileEnd(c);

	WCHAR f2[] = L"func2";
	ProfileBegin(f2);
	Test();
	ProfileEnd(f2);

	//WCHAR f3[] = L"func3";
	//ProfileBegin(f3);
	//Test();
	//ProfileEnd(f3);
	//ProfileBegin(f3);
	//Test();
	//ProfileEnd(f3);
	//ProfileBegin(f3);
	//Test();
	//ProfileEnd(f3);


	PrintProFile();
	ProfileDataOutText(L"Profile.txt");

	return 0;
}

void Test()
{
	for (int i = 0; i < 100000; i++) {}

	//printf("Test Done\n");
}