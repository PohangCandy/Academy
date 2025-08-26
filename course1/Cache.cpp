
//구조체 변수 초기화 시키는 작업은 c++11 이상에서 가능함.
//malloc으로 동적할당시 생성자 함수를 호출하지 않으므로 수동으로 초기화 해야함.
//그게 싫으면 new 사용



#include <stdio.h>
#include <stdlib.h>

//총 10개의 캐쉬인덱스를 가지는 캐쉬
//각 캐쉬라인이 가지는 변수
//캐쉬라인 인덱스[10], Tag(캐쉬라인 구별한 사위 비트), invalid(캐쉬라인이 할당됬는지 여부)

#define SIZEOFCACHEDATA 64

//캐쉬라인 인덱스 -> 6비트 정보를 저장하고 있는 변수, 2바이트 정보로 충분할 듯?, short로 담아준다.
typedef unsigned short SizeofcacheIndex;

//캐쉬라인 태그 -> 상위비트 정보를 저장라고 있는 변수, 하위 12비트를 제외한 20비트 저장할 공간, 4바이트로 지정
typedef unsigned int SizeofTag;

//캐쉬 데이터
//캐쉬라인 인덱스, Tag(캐쉬라인 구별한 사위 비트), invalid(캐쉬라인이 할당됬는지 여부)
struct  cacheData
{
	SizeofcacheIndex index;
	SizeofTag tag;
	bool invalid = false;
};

cacheData* CD;

//way까지 관리하려면 현재 캐쉬라인이 꽉찼는지 알아야하지 않을까?
int CDsize = 0;

//캐쉬 데이터 초기화
void initCacheData();

void Cache(void* a);

void printCacheTable();


int main()
{
	initCacheData();
	
	alignas(64) int arr[17];
	for (int i = 0; i < 17; ++i) {
		Cache(&arr[i]); // 서로 다른 주소
	}


	printCacheTable();
}

//캐쉬 데이터 초기화
void initCacheData()
{
	CD = (cacheData*)malloc(sizeof(cacheData) * SIZEOFCACHEDATA);
	for (int i = 0; i < SIZEOFCACHEDATA; ++i) {
		CD[i].invalid = false;
		CD[i].index = 0;
		CD[i].tag = 0;
	}
}

void Cache(void* a)
{
	unsigned int p = (unsigned int)a;
	//하위6비트가 0, 상위6비트가 1
	unsigned int index = ((p >> 6) & (SIZEOFCACHEDATA - 1));
	//unsigned int index = (p & 0xFC0) >> 6;
	unsigned int Tag = (p & 0xFFFFF000) >>12;

	//캐쉬에 해당 캐쉬라인 인덱스가 있을 경우
	if (CD[index].invalid)
	{
		//태그 조사하기
		//태그가 일치하는 경우 캐쉬 히트
		if (CD[index].tag == Tag)
		{
			printf("Cache Hit!\n");
			printf("pointer Index : %d\n", index);
			printf("pointer Tag : %d\n", Tag);
			printf("Cache Tag : %d\n", CD[index].tag);
		}
		//태그가 일치하지 않다면 캐쉬미스
		//해당 인덱스에 새롭게 캐쉬라인을 작성
		else
		{
			printf("Cache Miss!\n");
			printf("pointer Index : %d\n", index);
			printf("pointer Tag : %d\n", Tag);
			printf("Cache Tag : %d\n", CD[index].tag);
			CD[index].tag = Tag;
			CD[index].index = index;
		}
	}
	//캐쉬 데이터에 캐쉬라인 인덱스가 없는 경우
	//새롭게 인덱스 추가하기
	else
	{	
		printf("Cache Miss!\n");
		printf("pointer Index : %d\n", index);
		CD[index].tag = Tag;
		CD[index].index = index;
		CD[index].invalid = true;
	}
}

void printCacheTable()
{
	printf("|     Num   |     Index     |        Tag        |         Invalid    |\n");
	for (int i = 0; i < SIZEOFCACHEDATA; i++)
	{
		if (CD[i].invalid)
		{
			char Inv[10] = "true";
			printf("|      %2d  |      %x       |        %x        |           %s       |\n", i, CD[i].index, CD[i].tag, Inv);
		}
		else
		{
			char Inv[10] = "false";
			printf("|      %2d  |      --       |        --        |           %s       |\n", i,Inv);
		}
	}
}

