#include "MultiThreadProfiler.h"
#include <iostream>

//-----------------------------------------------------------------------
// 스레드 마다 시간 측정을 분리해 조사하기 위한 변수
//-----------------------------------------------------------------------
__declspec(thread) LARGE_INTEGER g_tls_lStartTime = { 0 };

//------------------------------------------------------
// 각 프로파일러를 저장해둘 배열
//------------------------------------------------------
static PROFILE_SAMPLE arP[PROFILE_NUM];

//------------------------------------------------------
//프로파일 이름으로 존재하는 프로파일 검색
//이미 있을 경우 해당 프로파일 정보 갱신
//------------------------------------------------------
PROFILE_SAMPLE* findExistProfile(WCHAR* szName)
{
	//이미 있을 경우 해당 프로파일 정보 갱신
	int pi = 0;
	while (pi < PROFILE_NUM)
	{
		if (arP[pi].lFlag && wcscmp(arP[pi].sxName, szName) == 0)
		{
			return &arP[pi];
		}
		pi++;
	}
	return nullptr;
}

//새로운 프로파일 들어갈 위치 검색
PROFILE_SAMPLE* FindemptyProFile()
{
	int proindex = 0;
	while (proindex < PROFILE_NUM)
	{
		if (arP[proindex].lFlag == 0)
		{
			return &arP[proindex];
		}
		proindex++;
	}

	return nullptr;
}

//새로운 프로파일러 초기화
void initPRoFile(PROFILE_SAMPLE* pf, WCHAR* szName)
{
	pf->lFlag = 1;
	//wcscpy(pf->sxName, szName);
	wcscpy_s(pf->sxName, sizeof(pf->sxName), szName);
	for (int i = 0; i < PROFILE_SAMPLE_MAX;i++)
	{
		pf->iMax[i] = 0;
	}
	for (int i = 0; i < PROFILE_SAMPLE_MIN;i++)
	{
		pf->iMin[i] = 100000000000LL;
	}
}

//ProFile 시간 측정 시작
void BeginTimeCount(PROFILE_SAMPLE* pf)
{
	if (!QueryPerformanceCounter(&g_tls_lStartTime))
	{
		bool fuck = true;
	}

	InterlockedIncrement((long*)&(pf->iCall));
	//pf->iCall++;
}

/////////////////////////////////////////////////////////////////////////////
// 하나의 함수 Profiling 시작, 끝 함수.
//
// Parameters: (char *)Profiling이름.
// Return: 없음.
/////////////////////////////////////////////////////////////////////////////
void ProfileBegin(WCHAR* szName)
{
	PROFILE_SAMPLE* newP = findExistProfile(szName);
	if (newP == nullptr)
	{
		newP = FindemptyProFile();
		if (newP != nullptr)
		{
			initPRoFile(newP, szName);
			BeginTimeCount(newP);
			return;
		}
	}
	else
	{
		BeginTimeCount(newP);
		return;
	}


	printf("Arr ProFile is Full\n");
}

void EndTimeCount(PROFILE_SAMPLE* pf)
{
	LARGE_INTEGER end;
	LARGE_INTEGER time;
	LARGE_INTEGER Freq;;

	QueryPerformanceCounter(&end);
	QueryPerformanceFrequency(&Freq);
	long long Result = end.QuadPart - g_tls_lStartTime.QuadPart;

	long long nanoResult = Result * 1000000000LL / Freq.QuadPart;

	//InterlockedExchangeAdd64(&(pf->iTotalTime), nanoResult);
	(pf->iTotalTime) += nanoResult;


	//측정한 시간을 최소 테이블과 최대 테이블에 비교해서 넣는다.
	long long copy_nanoResult = nanoResult;
	for (int i = 0; i < PROFILE_SAMPLE_MAX; i++)
	{
		long long temp = pf->iMax[i];

		if (pf->iMax[i] < copy_nanoResult)
		{
			pf->iMax[i] = copy_nanoResult;
			copy_nanoResult = temp;
		}
	}

	copy_nanoResult = nanoResult;
	for (int i = 0; i < PROFILE_SAMPLE_MIN; i++)
	{
		long long temp = pf->iMin[i];

		if (pf->iMin[i] > copy_nanoResult)
		{
			pf->iMin[i] = copy_nanoResult;
			copy_nanoResult = temp;
		}
	}
}

void ProfileEnd(WCHAR* szName)
{
	PROFILE_SAMPLE* pf = findExistProfile(szName);
	if (pf == nullptr)
	{
		wprintf(L"The %s is not exist\n", szName);
		return;
	}

	EndTimeCount(pf);
}


//--------------------------------------------------------------
// average도 너무 큰 최댓값이랑 최솟값을 제외한 후 구하자
//--------------------------------------------------------------
long long calculateAverage(PROFILE_SAMPLE* pf)
{
	long long totaltime = pf->iTotalTime;

	int numofExcept = 9;

	for (int i = 0; i < numofExcept; i++)
	{
		totaltime -= pf->iMax[i];
		totaltime -= pf->iMin[i];
	}

	long long icall = pf->iCall;
	icall -= numofExcept;

	return totaltime / icall;
}

//--------------------------------------------------------------------
//--------------------------------------------------------------------
void PrintProFile()
{
	printf("-------------------------------------------------------------------------------\n");
	printf("           Name  |     Average  |        Min   |        Max   |      Call |\n");
	printf("-------------------------------------------------------------------------------\n");

	for (int i = 0; i < PROFILE_NUM;i++)
	{
		PROFILE_SAMPLE* pf = &arP[i];
		if (pf->lFlag)
		{
			//long long average = (pf->iTotalTime) / (pf->iCall);
			long long average = calculateAverage(pf);
			wprintf(L"           %s  |     %lld  |        %lld   |        %lld   |      %d |\n", pf->sxName, average, pf->iMin[9], pf->iMax[9], pf->iCall);
			wprintf(L"-------------------------------------------------------------------------------\n");
		}
	}
}



/////////////////////////////////////////////////////////////////////////////
// Profiling 된 데이타를 Text 파일로 출력한다.
//
// Parameters: (char *)출력될 파일 이름.
// Return: 없음.
/////////////////////////////////////////////////////////////////////////////
void ProfileDataOutText(const WCHAR* szFileName)
{
	WCHAR s[256 * PROFILE_NUM] =
		L"-------------------------------------------------------------------------------\n"
		L"           Name  |     Average  |        Min   |        Max   |      Call |\n"
		L"-------------------------------------------------------------------------------\n";

	for (int i = 0; i < PROFILE_NUM;i++)
	{
		PROFILE_SAMPLE* pf = &arP[i];
		if (pf->lFlag)
		{
			WCHAR buffer[256];

			long long average = (pf->iTotalTime) / (pf->iCall);
			swprintf(buffer, 256, L"           %s  |     %lld  |        %lld   |        %lld   |      %d |\n"
				L"-------------------------------------------------------------------------------\n",
				pf->sxName, average, pf->iMin[9], pf->iMax[9], pf->iCall);

			size_t ls = wcslen(s);
			memcpy(&s[ls], buffer, (wcslen(buffer) + 1) * sizeof(WCHAR));
			//wprintf(L"%s\n", s);
		}
	}

	WCHAR cs[256];
	wcscpy_s(cs, szFileName);
	FILE* f;
	//f =_wfopen(cs, L"wt");
	_wfopen_s(&f, cs, L"wt");
	if (f != NULL)
	{
		fputws(s, f);
		fclose(f);
	}
}

///////////////////////////////////////////////////////////////////////////
 //프로파일링 된 데이터를 모두 초기화 한다.

 //Parameters: 없음.
 //Return: 없음.
///////////////////////////////////////////////////////////////////////////
void ProfileReset(void)
{
	for (int i = 0; i < PROFILE_NUM; i++)
	{
		if (arP[i].lFlag)
		{
			arP[i].lFlag = 0;
			arP[i].iTotalTime = 0;
		}
	}
}
