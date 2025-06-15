#define NOT_MY_NEW
#include "my_new.h"

////-----------------------------------------------------------------
//// 동적 할당 메모리 최대 할당 개수
////-----------------------------------------------------------------
#define MAXALLOCNUM 100

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
static int allocListTail;

//--------------------------------------------
// 파일을 저장해둘 변수
//--------------------------------------------
static FILE* file;

//------------------------------------------------------------------
// 파일에 넣기 전 에러 코드를 담을 메모리
//------------------------------------------------------------------
static char* filememory;

//--------------------------------------------
// 파일 메모리를 무제한으로 할당하기 위해 메모리 끝을 가리킬 tail
//--------------------------------------------
static int fileTail;

//-----------------------------------------------------
// 현재 날짜,시,분을 기준으로 로그 파일을 저장할 함수 
//-----------------------------------------------------
void makefile()
{
	static time_t t;
	static tm pt;

	time(&t);
    localtime_s(&pt ,&t);

	char buff[256];
	sprintf_s(buff, sizeof(buff), "Alloc_%04d%02d%02d_%02d%02d%02d.txt", pt.tm_year + 1900, pt.tm_mon + 1, pt.tm_mday, pt.tm_hour, pt.tm_min, pt.tm_sec);
	fopen_s(&file,buff, "wt");
	if (!file)
	{
		printf("파일 생성 실패\n");
		return;
	}

	//filememory = (char*)malloc(sizeof(char));
	filememory = nullptr;
	fileTail = 0;
}

//-----------------------------------------------------
// 로그를 메모리에 먼저 작성하기
//-----------------------------------------------------
void writefilememory(char log[],int size)
{
	//if (!filememory) return;
	//다음줄로 넘어가기 위한 메모리와 널 포인터를 위한 +2
	char* newmem  = (char*)realloc(filememory, fileTail + size + 2);
	if (!newmem) return;
	filememory = newmem;
	memcpy(filememory + fileTail, log, size);
	fileTail = fileTail + size;
	filememory[fileTail] = '\n';
	filememory[fileTail + 1] = '\0';
	fileTail++;
}

//-----------------------------------------------------
// 파일 닫기
// 지금까지 저장된 파일 메모리를 파일에 작성한다.
//-----------------------------------------------------
void closefile()
{
	if (filememory) {
		fwrite(filememory, fileTail, 1, file);
		free(filememory);
		filememory = nullptr;
	}
	fileTail = 0;

	fclose(file);
}


//-----------------------------------------------
// 할당 받을 메모리 정보를 저장할 함수
//-----------------------------------------------
void* operator new(size_t size, const char* File, int Line)
{
	if (allocListTail < MAXALLOCNUM - 1)
	{
		void* pv = malloc(size);
		if (pv != nullptr)
		{
			arrAllocList[allocListTail].ptr = pv;
			arrAllocList[allocListTail].size = size;
			strcpy_s(arrAllocList[allocListTail].filename, sizeof(arrAllocList[allocListTail].filename), File);
			arrAllocList[allocListTail].line = Line;
			arrAllocList[allocListTail].array = false;
			allocListTail++;
			return pv;
		}
	}

	return nullptr;
}

void* operator new[](size_t size, const char* File, int Line)
{
	if (allocListTail < MAXALLOCNUM - 1)
	{
		void* pv = malloc(size);
		//메타데이터도 넣어야되나?
		//메타 데이터 넣으면 배열인지 체크하는 불 변수 필요없어질듯

		if (pv != nullptr)
		{
			arrAllocList[allocListTail].ptr = pv;
			arrAllocList[allocListTail].size = size;
			strcpy_s(arrAllocList[allocListTail].filename, sizeof(arrAllocList[allocListTail].filename), File);
			arrAllocList[allocListTail].line = Line;
			arrAllocList[allocListTail].array = true;
			allocListTail++;
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

//-------------------------------------------------
// 할당하지 않은 메모리 해제 시도 로그
//--------------------------------------------------
void NOALLOC(const void* p)
{
	char buff[256];
	sprintf_s(buff, sizeof(buff), "NOALLOC [0x%p]", p);
	writefilememory(buff,strlen(buff));
	printf("%s\n", buff);
}

void ARRAY(const void* p, const int size, const char* filename, const int line)
{
	char buff[256];
	sprintf_s(buff, sizeof(buff), "ARRAY  [0x%p]  [%5d]  %s : %d",p,size,filename,line);
	writefilememory(buff, strlen(buff));
	printf("%s\n", buff);
}

void LEAK(const void* p, const int size, const char* filename, const int line)
{
	char buff[256];
	sprintf_s(buff, sizeof(buff), "LEAK    [0x%p]  [%5d]  %s : %d",p, size, filename, line);
	writefilememory(buff, strlen(buff));
	printf("%s\n", buff);
}