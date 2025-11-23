#pragma once

#ifdef PROFILE
#define PRO_BEGIN(TagName) ProfileBegin(TagName)
#define PRO_END(TagName) ProfileName(TagName)
#elseif
#define PRO_BEGIN(TagName)
#define PRO_END(TagName)
#endif

#include<windows.h>

#define PROFILE_NUM 10
#define PROFILE_SAMPLE_Name 64
#define PROFILE_SAMPLE_MIN 2
#define PROFILE_SAMPLE_MAX 2

struct PROFILE_SAMPLE {
	//프로파일 사용여부
	long lFlag = 0;
	//프로파일 샘플 이름
	WCHAR sxName[PROFILE_SAMPLE_Name];

	// 프로파일 샘플 실행 시간.
	LARGE_INTEGER lStartTime;

	// 전체 사용시간 카운터 Time.	(출력시 호출회수로 나누어 평균 구함)
	_int64 iTotalTime;

	//최소 사용시간 카운터 Time
	//(초단위로 계산하여 저장 / [0] 가장최소 [1] 다음 최소 [2])
	_int64 iMin[PROFILE_SAMPLE_MIN];

	//최대 사용시간 카운터 Time
	//(초단위로 계산하여 저장 / [0] 가장최대[1] 다음 최대[2])
	_int64 iMax[PROFILE_SAMPLE_MAX];

	// 누적 호출 횟수
	int iCall;
};

//이미 있을 경우 해당 프로파일 정보 갱신
PROFILE_SAMPLE* findExistProfile(WCHAR* szName);

//새로운 프로파일 들어갈 위치 검색
PROFILE_SAMPLE* FindemptyProFile();

//새로운 프로파일러 초기화
void initPRoFile(PROFILE_SAMPLE* pf, WCHAR* szName);

//ProFile 시간 측정 시작
void BeginTimeCount(PROFILE_SAMPLE* pf);

/////////////////////////////////////////////////////////////////////////////
// 하나의 함수 Profiling 시작, 끝 함수.
//
// Parameters: (char *)Profiling이름.
// Return: 없음.
/////////////////////////////////////////////////////////////////////////////
void ProfileBegin(WCHAR* szName);

void EndTimeCount(PROFILE_SAMPLE* pf);

void ProfileEnd(WCHAR* szName);

void PrintProFile();


/////////////////////////////////////////////////////////////////////////////
// Profiling 된 데이타를 Text 파일로 출력한다.
//
// Parameters: (char *)출력될 파일 이름.
// Return: 없음.
/////////////////////////////////////////////////////////////////////////////
void ProfileDataOutText(const WCHAR* szFileName);

/////////////////////////////////////////////////////////////////////////////
// 프로파일링 된 데이터를 모두 초기화 한다.
//
// Parameters: 없음.
// Return: 없음.
/////////////////////////////////////////////////////////////////////////////
void ProfileReset(void);
