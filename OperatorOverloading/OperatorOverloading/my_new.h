#pragma once

#include<iostream>

//1.ptr은 어캐 구함??
//-> 포인터 변수를 할당받고, 해당 포인터 주소를 리턴해준다.

//2.전역 배열, 전역 변수 맘에 안 듬.
//배열로 구현하려면 어쩔수 없어보임 -> 리스트로 수정해보기
//->일단 main이 아닌 my_new.cpp에 정적으로 선언해서 숨길 수 있음.

//3.객체를 저장할때 생성자나 소멸자 호출하려면 메타데이터 넣어줘야 하지 않을까?
// 그리고 내가 직접 객체의 생성자나 소멸자를 호출해야되나?
//->new 키워드에 의해 자동으로 함수, 연산자로서의 역할을 하고 있음.
//그래서 생성자랑 소멸자를 호출하거나 메타데이터 넣는 작업을 연산자가 자동으로 해주고 있음.

//4. 객체 배열일 경우 해제할때 delete로 잘못해제 시도 유무 판단하기
//	MyClass* pc = new MyClass[10];
// delete pc;
// 이렇게 할 경우 NOALLOC이 뜸 -> 메타데이터를 가리키도록 해야 ARRAY가 뜰 것
//	MyClass* pc = new MyClass;
// delete[] pc;
// 이렇게되면 delete하기 전에 소멸자를 무수히 호출
// Array가 예상되지만 소멸자때문에 결과확인 못함.

//5. 파일로 로그 저장하기

#define MAXALLOCNUM 100

//------------------------------------------------------------------
// 파일에 넣기 전 에러 코드를 담을 메모리
//------------------------------------------------------------------



struct __AllocInfo
{
	void* ptr;
	int size;
	char filename[128];
	int  line;
	bool array;
};

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

