#pragma once

#include<iostream>

#define MAXALLOCNUM 100

struct AllocInfo
{
	void* ptr;
	int size;
	char filename[128];
	int  line;
	bool array;
};

//전역 배열 맘에 안듬
AllocInfo arrAllocList[MAXALLOCNUM];

void* operator new (size_t size, char* File, int Line)
{
	//static말곤 방법이 없나?
	//전역 배열 끝에 들어갈 수 있는 좋은 방법을 찾거나
	//리스트로 구현하기
	static int tail = 0;
	if (tail < MAXALLOCNUM - 1)
	{
		//ptr은 어캐 구함??
		arrAllocList[tail].size = size;
		strcpy_s(arrAllocList[tail].filename,sizeof(arrAllocList[tail].filename),File);
		arrAllocList[tail].line = Line;
		tail++;
	}
}

void* operator new[](size_t size, char* File, int Line)
{

}

void operator delete (void* p, char* File, int Line)
{
}
void operator delete[](void* p, char* File, int Line)
{
}

// 실제로 사용할 delete	
void operator delete (void* p)
{
}
void operator delete[](void* p)
{
}





// 프로그램이 시작되면 파일을 만들고 
// 해당 파일에 들어갈 내용을 미리 메모리에 저장해두었다가
// 메모리에 내용이 적혀 있다면 로그 파일을 출력한다.

//-----------------------------------------------------
// 현재 날짜,시,분을 기준으로 로그 파일을 저장할 함수 
//-----------------------------------------------------

