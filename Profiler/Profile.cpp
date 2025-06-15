#include "Profile.h"
#include <iostream>

////굳이 static으로 숨길 필요있을까?
PROFILE_SAMPLE arP[PROFILE_NUM];

//이미 있을 경우 해당 프로파일 정보 갱신
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
		pf->iMin[i] = 100000000000;
	}
}

//ProFile 시간 측정 시작
void BeginTimeCount(PROFILE_SAMPLE* pf)
{
	QueryPerformanceCounter(&(pf->lStartTime));
	pf->iCall++;
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
	long long Result = end.QuadPart - (pf->lStartTime).QuadPart;

	long long nanoResult = Result * 1000000000LL / Freq.QuadPart;
	(pf->iTotalTime) += nanoResult;

//todo
//측정한 시간을 최소 테이블과 최대 테이블에 비교해서 넣는다.
	for (int i = 0; i < PROFILE_SAMPLE_MAX; i++)
	{
		if (pf->iMax[i] < nanoResult)
		{
			pf->iMax[i] = nanoResult;
		}
	}

	for (int i = 0; i < PROFILE_SAMPLE_MIN; i++)
	{
		if (pf->iMin[i] > nanoResult)
		{
			pf->iMin[i] = nanoResult;
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

//--------------------------------------------------------------------
//--------------------------------------------------------------------
void PrintProFile()
{
	printf("-------------------------------------------------------------------------------\n");
	printf("           Name  |     Average  |        Min   |        Max   |      Call |\n");
	printf("-------------------------------------------------------------------------------\n");

	for(int i = 0; i < PROFILE_NUM;i++)
	{
		PROFILE_SAMPLE* pf = &arP[i];
		if (pf->lFlag)
		{
			long long average = (pf->iTotalTime) / (pf->iCall);
			wprintf(L"           %s  |     %lld  |        %lld   |        %lld   |      %d |\n", pf->sxName, average, pf->iMin[0], pf->iMax[0], pf->iCall);
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
				pf->sxName, average, pf->iMin[0], pf->iMax[0], pf->iCall);

			size_t ls = wcslen(s);
			memcpy(&s[ls], buffer, (wcslen(buffer) + 1)*sizeof(WCHAR));
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
		}
	}
}
