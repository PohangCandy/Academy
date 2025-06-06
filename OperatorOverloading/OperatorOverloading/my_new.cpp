#define NOT_MY_NEW
#include "my_new.h"

//-------------------------------------------
// my_new.h에 걸려있는 new 매크로 제거
//-------------------------------------------
#undef new

//---------------------------------------------------------------
// 할당된 메모리 정보 저장할 배열
//---------------------------------------------------------------
static __AllocInfo arrAllocList[MAXALLOCNUM];

//-----------------------------------------------------------------------------------
// 소멸자를 통해 프로그램 종료시 자동으로 해제되지 않은 메모리 체크할 전역 클래스
//-----------------------------------------------------------------------------------
static cCheakLeak cl;

//-------------------------------------------------------------------------
//배열과 변수로 각각 할당받을 수 있도록 구조체 배열 인덱스 끝을 가리킬 변수
//-------------------------------------------------------------------------
static int tail = 0;

//-----------------------------------------------
// 할당 받을 메모리 정보를 저장할 함수
//-----------------------------------------------
void* operator new(size_t size, const char* File, int Line)
{
	if (tail < MAXALLOCNUM - 1)
	{
		void* pv = malloc(size);
		if (pv != nullptr)
		{
			arrAllocList[tail].ptr = pv;
			arrAllocList[tail].size = size;
			strcpy_s(arrAllocList[tail].filename, sizeof(arrAllocList[tail].filename), File);
			arrAllocList[tail].line = Line;
			arrAllocList[tail].array = false;
			tail++;
			return pv;
		}
	}

	return nullptr;
}

void* operator new[](size_t size, const char* File, int Line)
{
	if (tail < MAXALLOCNUM - 1)
	{
		void* pv = malloc(size);
		//메타데이터도 넣어야되나?
		//메타 데이터 넣으면 배열인지 체크하는 불 변수 필요없어질듯

		if (pv != nullptr)
		{
			arrAllocList[tail].ptr = pv;
			arrAllocList[tail].size = size;
			strcpy_s(arrAllocList[tail].filename, sizeof(arrAllocList[tail].filename), File);
			arrAllocList[tail].line = Line;
			arrAllocList[tail].array = true;
			tail++;
			return pv;
		}
	}

	return nullptr;
}

void operator delete (void* p, const char* File, int Line)
{
}

void operator delete[](void* p, const char* File, int Line)
{
}

// 실제로 사용할 delete	
void operator delete(void* p)
{
	//NOALLOC
	if (p == nullptr) return;

	for (int i = 0; i < MAXALLOCNUM; i++)
	{
		if (arrAllocList[i].ptr == p)
		{
			//ARRAY
			if (arrAllocList[i].array)
			{
				ARRAY(arrAllocList[i].ptr, arrAllocList[i].size, arrAllocList[i].filename, arrAllocList[i].line);
				return;
			}
			free(p);
			arrAllocList[i] = { 0, };
			return;
		}
	}

	//NOALLOC
	NOALLOC(p);
	return;
}
void operator delete[](void* p)
{
	//NOALLOC
	if (p == nullptr) return;

	for (int i = 0; i < MAXALLOCNUM; i++)
	{
		if (arrAllocList[i].ptr == p)
		{
			//ARRAY
			if (!arrAllocList[i].array)
			{
				ARRAY(arrAllocList[i].ptr, arrAllocList[i].size,arrAllocList[i].filename,arrAllocList[i].line);
				return;
			}
			free(p);
			arrAllocList[i] = { 0, };
			return;
		}
	}

	//NOALLOC
	NOALLOC(p);
	return;
}

//---------------------------------------------
// 프로그램이 종료되기 전에 구조체 배열 순회하면서 해제안된 메모리 체크
//---------------------------------------------
void checkLeak()
{
	for (int i = 0; i < MAXALLOCNUM; i++)
	{
		if (arrAllocList[i].ptr != nullptr)
		{
			LEAK(arrAllocList[i].ptr, arrAllocList[i].size, arrAllocList[i].filename, arrAllocList[i].line);
		}
	}
}

void NOALLOC(const void* p)
{
	char buff[256];
	sprintf_s(buff, sizeof(buff), "NOALLOC [0x%p]", p);
	printf("%s\n", buff);
}

void ARRAY(const void* p, const int size, const char* filename, const int line)
{
	char buff[256];
	sprintf_s(buff, sizeof(buff), "ARRAY  [0x%p]  [%5d]  %s : %d",p,size,filename,line);
	printf("%s\n", buff);
}

void LEAK(const void* p, const int size, const char* filename, const int line)
{
	char buff[256];
	sprintf_s(buff, sizeof(buff), "LEAK    [0x%p]  [%5d]  %s : %d",p, size, filename, line);
	printf("%s\n", buff);
}