#pragma once

#include<iostream>

//1.ptr은 어캐 구함??
//포인터 변수를 할당받고, 해당 포인터 주소를 리턴해준다.

#define MAXALLOCNUM 100

//------------------------------------------------------------------
// 에러 코드를 담을 메모리
//------------------------------------------------------------------

struct __AllocInfo
{
	void* ptr;
	int size;
	char filename[128];
	int  line;
	bool array;
};

//전역 배열 맘에 안듬
//extern __AllocInfo arrAllocList[MAXALLOCNUM];
//static __AllocInfo arrAllocList[MAXALLOCNUM];

//전역 변수 밖에 방법없나?
//배열과 변수로 각각 할당받을 수 있도록 구조체 배열 인덱스 끝을 가리킬 변수
//extern int tail = 0;
//static int tail = 0;

//-----------------------------------------------
// 할당 받을 메모리 정보를 저장할 함수
//-----------------------------------------------
void* operator new(size_t size, const char* File, int Line);


void* operator new[](size_t size, const char* File, int Line);


void operator delete (void* p, const char* File, int Line);

void operator delete[](void* p, const char* File, int Line);

// 실제로 사용할 delete	
void operator delete(void* p);

void operator delete[](void* p);

void NOALLOC(const void* p);

void ARRAY(const void* p, const int size, const char* filename, const int line);

void LEAK(const void* p, const int size, const char* filename, const int line);

//---------------------------------------------
// 프로그램이 종료되기 전에 구조체 배열 순회하면서 해제안된 메모리 체크
//---------------------------------------------
void checkLeak();

//-------------------------------------
//해제 안된 메모리 찾기 위한 클래스 
//-------------------------------------
class cCheakLeak {
public:
	cCheakLeak() {};
	~cCheakLeak() {
		checkLeak();
	};
};

//------------------------------------------------- 
// main.cpp에서 헤더 포함 시 자동으로 새로운 new 적용
//-------------------------------------------------
#if defined(NOT_MY_NEW)
#else
#define new new( __FILE__ , __LINE__)
#endif // defined(NOT_MY_NEW)


// 프로그램이 시작되면 파일을 만들고 
// 해당 파일에 들어갈 내용을 미리 메모리에 저장해두었다가
// 메모리에 내용이 적혀 있다면 로그 파일을 출력한다.

//-----------------------------------------------------
// 현재 날짜,시,분을 기준으로 로그 파일을 저장할 함수 
//-----------------------------------------------------

